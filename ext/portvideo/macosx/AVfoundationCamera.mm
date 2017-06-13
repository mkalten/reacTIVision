/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "AVfoundationCamera.h"
#include "CameraTool.h"

#ifdef OSC_HOST_BIG_ENDIAN
#define FourCC2Str(fourcc) (const char[]){*((char*)&fourcc), *(((char*)&fourcc)+1), *(((char*)&fourcc)+2), *(((char*)&fourcc)+3),0}
#else
#define FourCC2Str(fourcc) (const char[]){*(((char*)&fourcc)+3), *(((char*)&fourcc)+2), *(((char*)&fourcc)+1), *(((char*)&fourcc)+0),0}
#endif

@implementation FrameGrabber
- (id) initWithCameraSize:(int)cw :(int)ch :(int) bytes
{
    self = [super init];
    cam_width = cw;
    cam_height = ch;
    new_frame = false;
    crop = false;
    
    if(bytes==3) color = true;
    else color = false;
    
    buffer = new unsigned char[cam_width*cam_height*bytes];
    return self;
}

- (id) initWithCropSize:(int)cw :(int)ch :(int)bytes :(int)fw :(int)fh :(int)xo :(int)yo
{
    self = [super init];
    cam_width =  cw;
    cam_height = ch;
    frm_width =  fw;
    frm_height = fh;
    xoff = xo;
    yoff = yo;
    new_frame = false;
    crop = true;
    
    if(bytes==3) color = true;
    else color = false;
    
    buffer = new unsigned char[frm_width*frm_height*bytes];
    return self;
}


- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    
    new_frame = false;
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (CVPixelBufferLockBaseAddress(imageBuffer, 0) == kCVReturnSuccess) {
        
        unsigned char *src = (unsigned char*)CVPixelBufferGetBaseAddress(imageBuffer);
        unsigned char *dest = buffer;
        
        if (color) {
            
            if (crop) {
                unsigned char *src_buf = src + 3*(yoff*cam_width + xoff);
                
                for (int i=0;i<frm_height;i++) {
                    memcpy(dest, src_buf, 3*frm_width);
                    
                    src_buf += 3*cam_width;
                    dest += 3*frm_width;
                }
                
            } else {
                
                memcpy(dest,src,cam_width*cam_height*3);
            }
            
        } else {
            
            if (crop) {
                
                src += 2*(yoff*cam_width);
                int xend = (cam_width-(frm_width+xoff));
                
                for (int i=0;i<frm_height;i++) {
                    
                    src +=  2*xoff;
                    for (int j=frm_width/2;j>0;j--) {
                        *dest++ = *src++;
                        src++;
                        *dest++ = *src++;
                        src++;
                    }
                    src +=  2*xend;
                    
                }
                
            } else {
                
                int size = cam_width*cam_height/2;
                for (int i=size;i>0;i--) {
                    *dest++ = *src++;
                    src++;
                    *dest++ = *src++;
                    src++;
                }
            }
        }
        
        new_frame = true;
    }
}

- (unsigned char*) getFrame
{
    if (new_frame){
        new_frame = false;
        return buffer;
    } else return NULL;
}


- (void)dealloc
{
    if(buffer) delete []buffer;
    [super dealloc];
}
@end

AVfoundationCamera::AVfoundationCamera(CameraConfig *cam_cfg):CameraEngine(cam_cfg)
{
    disconnected = false;
    running=false;
    lost_frames=0;
    timeout = 1000;
    
    uvcController = NULL;
    
    session = NULL;
    videoDeviceInput = NULL;
    videoOutput = NULL;
    
    videoDevice = NULL;
    grabber = NULL;
	
	cam_cfg->driver = DRIVER_DEFAULT;
}

AVfoundationCamera::~AVfoundationCamera()
{
    if (uvcController) [uvcController release];
    if (videoDevice)   [videoDevice release];
    if (videoOutput)   [videoOutput release];
    if (grabber)       [grabber release];
}

int AVfoundationCamera::getDeviceCount() {
	
	NSArray *dev_list0 = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	NSArray *dev_list1 = [AVCaptureDevice devicesWithMediaType:AVMediaTypeMuxed];
	return [dev_list0 count] + [dev_list1 count];
}

std::vector<CameraConfig> AVfoundationCamera::getCameraConfigs(int dev_id) {
    std::vector<CameraConfig> cfg_list;

    int dev_count = getDeviceCount();
    if (dev_count==0) return cfg_list;
	
    NSMutableArray *captureDevices = [NSMutableArray arrayWithCapacity:dev_count];
	NSArray *dev_list0 = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (AVCaptureDevice* device in dev_list0) [captureDevices addObject:device];
	NSArray *dev_list1 = [AVCaptureDevice devicesWithMediaType:AVMediaTypeMuxed];
    for (AVCaptureDevice* device in dev_list1)[captureDevices addObject:device];
    
    int cam_id = -1;
    for (AVCaptureDevice* device in captureDevices) {
		cam_id ++;
		if ((dev_id>=0) && (dev_id!=cam_id)) continue;
        
        CameraConfig cam_cfg;
        CameraTool::initCameraConfig(&cam_cfg);
        
        cam_cfg.driver = DRIVER_DEFAULT;
        cam_cfg.device = cam_id;

        if ([device localizedName]!=NULL)
            sprintf(cam_cfg.name,"%s",[[device localizedName] cStringUsingEncoding:NSUTF8StringEncoding]);
        else sprintf(cam_cfg.name,"unknown device");
		
		std::vector<CameraConfig> fmt_list;
		int last_format = FORMAT_UNKNOWN;
        NSArray *captureDeviceFormats = [device formats];
		for (AVCaptureDeviceFormat *format in captureDeviceFormats) {
						
            int32_t codec = CMVideoFormatDescriptionGetCodecType((CMVideoFormatDescriptionRef)[format formatDescription]);
			
			if (codec == '420f') codec = '420v';
			
			cam_cfg.cam_format = FORMAT_UNKNOWN;
			for (int i=FORMAT_MAX;i>0;i--) {
				if (codec == codec_table[i]) {
					cam_cfg.cam_format = i;
					break;
				}
			}
			
			if (cam_cfg.cam_format != last_format) {
				std::sort(fmt_list.begin(), fmt_list.end());
				cfg_list.insert( cfg_list.end(), fmt_list.begin(), fmt_list.end() );
				fmt_list.clear();
				last_format = cam_cfg.cam_format;
			}
			
            CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions((CMVideoFormatDescriptionRef)[format formatDescription]);
            
            cam_cfg.cam_width = dim.width;
            cam_cfg.cam_height = dim.height;
            
            for (AVFrameRateRange *frameRateRange in [format videoSupportedFrameRateRanges]) {
                cam_cfg.cam_fps = roundf([frameRateRange maxFrameRate]*10)/10.0f;
                fmt_list.push_back(cam_cfg);
            }
        }
		std::sort(fmt_list.begin(), fmt_list.end());
		cfg_list.insert( cfg_list.end(), fmt_list.begin(), fmt_list.end() );
    }
	
	[captureDevices release];
	return cfg_list;
}

CameraEngine* AVfoundationCamera::getCamera(CameraConfig *cam_cfg) {
		
	int dev_count = getDeviceCount();
	if (dev_count==0) return NULL;
	
	if ((cam_cfg->device==SETTING_MIN) || (cam_cfg->device==SETTING_DEFAULT)) cam_cfg->device=0;
	else if (cam_cfg->device==SETTING_MAX) cam_cfg->device=dev_count-1;
	
	std::vector<CameraConfig> cfg_list = AVfoundationCamera::getCameraConfigs(cam_cfg->device);
	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);
	
	if (cam_cfg->force) return new AVfoundationCamera(cam_cfg);
	
	for (int i=0;i<cfg_list.size();i++) {
		
		if (cam_cfg->cam_format != cfg_list[i].cam_format) continue;
		if ((cam_cfg->cam_width >=0) && (cam_cfg->cam_width != cfg_list[i].cam_width)) continue;
		if ((cam_cfg->cam_height >=0) && (cam_cfg->cam_height != cfg_list[i].cam_height)) continue;
		if ((cam_cfg->cam_fps >=0) && (cam_cfg->cam_fps != cfg_list[i].cam_fps)) continue;

		return new AVfoundationCamera(cam_cfg);
	}
	
	return NULL;
}

bool AVfoundationCamera::initCamera() {
	
    int dev_count = getDeviceCount();
    if ((dev_count==0) || (cfg->device < 0) || (cfg->device>=dev_count)) return false;
	
    NSMutableArray *videoDevices = [NSMutableArray arrayWithCapacity:dev_count];
	NSArray *dev_list0 = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice* dev in dev_list0) [videoDevices addObject:dev];
	NSArray *dev_list1 = [AVCaptureDevice devicesWithMediaType:AVMediaTypeMuxed];
    for (AVCaptureDevice* dev in dev_list1) [videoDevices addObject:dev];
	
    videoDevice = [videoDevices objectAtIndex:cfg->device];
    if (videoDevice==NULL) return false;
    else [videoDevices release];
	
    if ([videoDevice localizedName]!=NULL)
        sprintf(cfg->name,"%s",[[videoDevice localizedName] cStringUsingEncoding:NSUTF8StringEncoding]);
    else sprintf(cfg->name,"unknown");
    
    session = [[AVCaptureSession alloc] init];
    if (session==NULL) return false;
    
    NSError *error = nil;
    videoDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
    if (videoDeviceInput == NULL) return false;

	std::vector<CameraConfig> cfg_list = getCameraConfigs(cfg->device);
	if (cfg->cam_format==FORMAT_UNKNOWN) cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cfg, cfg_list);
	
    AVCaptureDeviceFormat *selectedFormat = NULL;
    AVFrameRateRange *selectedFrameRateRange = NULL;
	NSArray *videoDeviceFormats = [videoDevice formats];
    for (AVCaptureDeviceFormat *format in videoDeviceFormats) {
        
        CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions((CMVideoFormatDescriptionRef)[format formatDescription]);

        if ((dim.width!=cfg->cam_width) || (dim.height!=cfg->cam_height)) continue; // wrong size
        
        int cam_format=0;
        int32_t codec = CMVideoFormatDescriptionGetCodecType((CMVideoFormatDescriptionRef)[format formatDescription]);
		if (codec == '420f') codec = '420v';

		for (int i=FORMAT_MAX;i>0;i--) {

			if ((codec == codec_table[i]) && (cfg->cam_format==i)) {
				cam_format = i;
				break;
			}
		}
		
        if (!cam_format) continue; // wrong format
		else selectedFormat = format;
        
        for (AVFrameRateRange *frameRateRange in [selectedFormat videoSupportedFrameRateRanges]) {
            float framerate = roundf([frameRateRange maxFrameRate]*10)/10.0f;
            if (framerate==cfg->cam_fps) { // found exact framerate
                selectedFrameRateRange = frameRateRange;
				break;
            }
        }
        
        if (selectedFrameRateRange) break;
    }
    
    if ((selectedFrameRateRange==NULL) || (selectedFormat==NULL)) return false;
	
    [videoDevice lockForConfiguration:&error];
    [videoDevice setActiveFormat:selectedFormat];
    if ([[[videoDevice activeFormat] videoSupportedFrameRateRanges] containsObject:selectedFrameRateRange]) {
        //[videoDevice setActiveVideoMaxFrameDuration:[selectedFrameRateRange maxFrameDuration]];
        [videoDevice setActiveVideoMinFrameDuration:[selectedFrameRateRange minFrameDuration]];
	}
    [videoDevice unlockForConfiguration];
	
    CMVideoDimensions dimensions =
    CMVideoFormatDescriptionGetDimensions((CMVideoFormatDescriptionRef)[[videoDevice activeFormat] formatDescription]);
    
    cfg->cam_width =  dimensions.width;
    cfg->cam_height = dimensions.height;
	
    for (AVFrameRateRange *frameRateRange in [[videoDevice activeFormat] videoSupportedFrameRateRanges])
    {
        cfg->cam_fps = roundf([frameRateRange maxFrameRate]*10)/10.0f;
		if (CMTIME_COMPARE_INLINE([frameRateRange minFrameDuration], ==, [videoDevice activeVideoMinFrameDuration])) {
			break;
		}
    }
    
    videoOutput = [[AVCaptureVideoDataOutput alloc] init];
    
    unsigned int pixelformat = kCVPixelFormatType_422YpCbCr8_yuvs;
	cfg->src_format = FORMAT_YUYV;
	if (cfg->color) {
		pixelformat = kCVPixelFormatType_24RGB;
		cfg->src_format = FORMAT_RGB;
	}
    
    NSDictionary *pixelBufferOptions = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithDouble:cfg->cam_width], (id)kCVPixelBufferWidthKey,
                                        [NSNumber numberWithDouble:cfg->cam_height], (id)kCVPixelBufferHeightKey,
                                        [NSNumber numberWithUnsignedInt: pixelformat ],
										(id)kCVPixelBufferPixelFormatTypeKey, nil];
	
    [videoOutput setVideoSettings:pixelBufferOptions];
    
    videoOutput.alwaysDiscardsLateVideoFrames = YES;
    [session addInput:videoDeviceInput];
    [session addOutput:videoOutput];
    
    // configure output.
    setupFrame();
    dispatch_queue_t queue = dispatch_queue_create("queue", NULL);
    if (cfg->frame) {
        grabber = [[FrameGrabber alloc] initWithCropSize:cfg->cam_width :cfg->cam_height :cfg->buf_format :cfg->frame_width :cfg->frame_height :cfg->frame_xoff :cfg->frame_yoff];
    } else {
        grabber = [[FrameGrabber alloc] initWithCameraSize:cfg->cam_width :cfg->cam_height: cfg->buf_format];
    }
    [videoOutput setSampleBufferDelegate:grabber queue:queue];
    dispatch_release(queue);
    
    NSString *uniqueID = [videoDevice uniqueID];
    if (uniqueID!=NULL) {
        uvcController = [[VVUVCController alloc] initWithDeviceIDString:[videoDevice uniqueID]];
        if (uvcController) [uvcController resetParamsToDefaults];
    } // else std::cout << "VVUVCController NULL" << std::endl;

    return true;
}

unsigned char* AVfoundationCamera::getFrame()
{
    unsigned char *cambuffer = [grabber getFrame];
    if (cambuffer!=NULL) {
        timeout=100;
        lost_frames=0;
        return cambuffer;
    } else {
        usleep(10000);
        lost_frames++;
        if (lost_frames>timeout) {
            disconnected=true;
            running=false;
        }
        return NULL;
    }
    
    return NULL;
}

bool AVfoundationCamera::startCamera()
{
    [session startRunning];
	applyCameraSettings();
    running = true;
    return true;
}

bool AVfoundationCamera::stopCamera()
{
    [session stopRunning];
    running=false;
    return true;
}

bool AVfoundationCamera::stillRunning() {
    return running;
}

bool AVfoundationCamera::resetCamera()
{
    return (stopCamera() && startCamera());
}

bool AVfoundationCamera::closeCamera()
{
    if ((uvcController) && (!disconnected)) {
        updateSettings();
        CameraTool::saveSettings();
    }
    [session release];
    return true;
}

bool AVfoundationCamera::showSettingsDialog(bool lock) {
    
    if (uvcController) {
        [uvcController closeSettingsWindow];
        [uvcController openSettingsWindow];
    }
    
    return lock;
}

bool AVfoundationCamera::hasCameraSettingAuto(int mode) {
    
    if (uvcController==NULL) return false;
    
    switch (mode) {
        case EXPOSURE:
            return [uvcController autoExposureModeSupported];
        case WHITE:
            return [uvcController autoWhiteBalanceSupported];
        case FOCUS:
            return [uvcController autoFocusSupported];
        case COLOR_HUE:
            return [uvcController autoHueSupported];
    }
    
    return false;
}

bool AVfoundationCamera::getCameraSettingAuto(int mode) {
    
    if (uvcController==NULL) return false;
    if (!hasCameraSettingAuto(mode)) return false;
    
    switch (mode) {
        case EXPOSURE:
            if ([uvcController autoExposureMode]>UVC_AEMode_Manual) return true;
            else return false;
        case WHITE:
            return [uvcController autoWhiteBalance];
        case FOCUS:
            return [uvcController autoFocus];
        case COLOR_HUE:
            return [uvcController autoHue];
    }
    
    return false;
}

bool AVfoundationCamera::setCameraSettingAuto(int mode, bool flag) {
    
    if (uvcController==NULL) return false;
    if (!hasCameraSettingAuto(mode)) return false;
	
    switch (mode) {
        case EXPOSURE:
            if (flag==true) [uvcController setAutoExposureMode:UVC_AEMode_Auto];
			else [uvcController setAutoExposureMode:UVC_AEMode_Manual];
            return true;
        case WHITE:
            [uvcController setAutoWhiteBalance:flag];
            return true;
        case FOCUS:
            [uvcController setAutoFocus:flag];
            return true;
        case COLOR_HUE:
            [uvcController setAutoHue:flag];
            return true;
    }
    
    return false;
}

bool AVfoundationCamera::hasCameraSetting(int mode) {
    
    if (uvcController==NULL) return false;
    
    switch (mode) {
        case BRIGHTNESS:	return [uvcController brightSupported];
        case CONTRAST:		return [uvcController contrastSupported];
        case SHARPNESS:		return [uvcController sharpnessSupported];
		case GAMMA:			return [uvcController gammaSupported];
        case GAIN:			return [uvcController gainSupported];
        case AUTO_GAIN:		return hasCameraSettingAuto(GAIN);
        case EXPOSURE:		return [uvcController exposureTimeSupported];
        case AUTO_EXPOSURE:	return hasCameraSettingAuto(EXPOSURE);
        case FOCUS:			return [uvcController focusSupported];
        case AUTO_FOCUS:	return hasCameraSettingAuto(FOCUS);
        case WHITE:			return [uvcController whiteBalanceSupported];
        case AUTO_WHITE:	return hasCameraSettingAuto(WHITE);
        case BACKLIGHT:		return [uvcController backlightSupported];
		case SATURATION:	return [uvcController saturationSupported];
        case COLOR_HUE:		return [uvcController hueSupported];
        case AUTO_HUE:		return hasCameraSettingAuto(COLOR_HUE);
		case POWERLINE:		return [uvcController powerLineSupported];
    }
	
    return false;
}

bool AVfoundationCamera::setCameraSetting(int mode, int setting) {
    
    if (uvcController==NULL) return false;
    if (!hasCameraSetting(mode)) return false;
	setCameraSettingAuto(mode, false);
	
    switch (mode) {
        case BRIGHTNESS:	[uvcController setBright:setting]; return true;
        case CONTRAST:		[uvcController setContrast:setting]; return true;
        case SHARPNESS:		[uvcController setSharpness:setting]; return true;
        case GAIN:			[uvcController setGain:setting]; return true;
		case GAMMA:			[uvcController setGamma:setting]; return true;
        case EXPOSURE:		[uvcController setExposureTime:setting]; return true;
        case FOCUS:			[uvcController setFocus:setting]; return true;
        case WHITE:			[uvcController setWhiteBalance:setting]; return true;
        case BACKLIGHT:		[uvcController setBacklight:setting]; return true;
		case SATURATION:	[uvcController setSaturation:setting]; return true;
        case COLOR_HUE:		[uvcController setHue:setting]; return true;
		case POWERLINE:		[uvcController setPowerLine:setting]; return true;
    }
	
    return false;
}

int AVfoundationCamera::getCameraSetting(int mode) {
    
    if (uvcController==NULL) return 0;
    if (!hasCameraSetting(mode)) return 0;
    //if (getCameraSettingAuto(mode)) return 0;
    
    switch (mode) {
        case BRIGHTNESS:	return [uvcController bright];
        case CONTRAST:		return [uvcController contrast];
        case SHARPNESS:		return [uvcController sharpness];
        case GAIN:			return [uvcController gain];
		case GAMMA:			return [uvcController gamma];
        case EXPOSURE:		return [uvcController exposureTime];
        case FOCUS:			return [uvcController focus];
        case WHITE:			return [uvcController whiteBalance];
        case BACKLIGHT:		return [uvcController backlight];
		case SATURATION:	return [uvcController saturation];
        case COLOR_HUE:		return [uvcController hue];
		case POWERLINE:		return [uvcController powerLine];
    }
	
    return 0;
}

int AVfoundationCamera::getMaxCameraSetting(int mode) {
    
    if (uvcController==NULL) return 0;
    if (!hasCameraSetting(mode)) return 0;
    //if (getCameraSettingAuto(mode)) return 0;
    
    switch (mode) {
        case BRIGHTNESS:    return [uvcController maxBright];
        case CONTRAST:      return [uvcController maxContrast];
        case SHARPNESS:     return [uvcController maxSharpness];
        case GAIN:          return [uvcController maxGain];
		case GAMMA:         return [uvcController maxGamma];
        case EXPOSURE:      return [uvcController maxExposureTime];
        case FOCUS:         return [uvcController maxFocus];
        case WHITE:         return [uvcController maxWhiteBalance];
        case BACKLIGHT:     return [uvcController maxBacklight];
		case SATURATION:	return [uvcController maxSaturation];
        case COLOR_HUE:     return [uvcController maxHue];
		case POWERLINE:		return [uvcController maxPowerLine];
    }
    
    return 0;
}

int AVfoundationCamera::getMinCameraSetting(int mode) {
    
    if (uvcController==NULL) return 0;
    if (!hasCameraSetting(mode)) return 0;
    //if (getCameraSettingAuto(mode)) return 0;
    
    switch (mode) {
        case BRIGHTNESS:    return [uvcController minBright];
        case CONTRAST:      return [uvcController minContrast];
        case GAIN:          return [uvcController minGain];
		case GAMMA:         return [uvcController minGamma];
        case EXPOSURE:      return [uvcController minExposureTime];
        case SHARPNESS:     return [uvcController minSharpness];
        case FOCUS:         return [uvcController minFocus];
        case WHITE:         return [uvcController minWhiteBalance];
        case BACKLIGHT:     return [uvcController minBacklight];
		case SATURATION:	return [uvcController minSaturation];
		case COLOR_HUE:     return [uvcController minHue];
		case POWERLINE:		return [uvcController minPowerLine];
    }
    
    return 0;
}

bool AVfoundationCamera::setDefaultCameraSetting(int mode) {
    
    if (uvcController==NULL) return false;
    if (!hasCameraSetting(mode)) return false;
	setCameraSettingAuto(mode, false);

    switch (mode) {
        case BRIGHTNESS:
            [uvcController resetBright];
            default_brightness = [uvcController bright];
            break;
        case CONTRAST:
            [uvcController resetContrast];
            default_contrast = [uvcController contrast];
            break;
        case SHARPNESS:
            [uvcController resetSharpness];
            default_sharpness = [uvcController sharpness];
            break;
        case GAIN:
            [uvcController resetGain];
            default_gain = [uvcController gain];
            break;
		case GAMMA:
			[uvcController resetGamma];
			default_gamma = [uvcController gamma];
			break;
        case EXPOSURE:
            [uvcController resetExposureTime];
            default_exposure = [uvcController exposureTime];
            break;
        case FOCUS:
            [uvcController resetFocus];
            default_focus = [uvcController focus];
            break;
        case WHITE:
            [uvcController resetWhiteBalance];
            default_white = [uvcController whiteBalance];
            break;
        case BACKLIGHT:
            [uvcController resetBacklight];
            default_backlight = [uvcController backlight];
            break;
		case SATURATION:
			[uvcController resetSaturation];
			default_saturation = [uvcController saturation];
			break;
		case COLOR_HUE:
            [uvcController resetHue];
            default_hue = [uvcController hue];
            break;
		case POWERLINE:
			[uvcController resetPowerLine];
			default_powerline = [uvcController powerLine];
			break;
    }
	
    return false;
}

int AVfoundationCamera::getDefaultCameraSetting(int mode) {
    
    if (uvcController==NULL) return 0;
    if (!hasCameraSetting(mode)) return 0;
    
    switch (mode) {
        case BRIGHTNESS: return default_brightness;
        case CONTRAST: return default_contrast;
        case GAIN: return default_gain;
		case GAMMA: return default_gamma;
        case EXPOSURE: return default_exposure;
        case SHARPNESS: return default_sharpness;
        case FOCUS: return default_focus;
        case WHITE: return default_white;
        case BACKLIGHT: return default_backlight;
		case SATURATION: return default_saturation;
        case COLOR_HUE: return default_hue;
		case POWERLINE: return default_powerline;
    }
	
    return 0;
}

int AVfoundationCamera::getCameraSettingStep(int mode) {
    if (!hasCameraSetting(mode)) return 0;
    return 1;
}
