/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "QTKitCamera.h"
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
- (void)captureOutput:(QTCaptureOutput *)captureOutput
  didOutputVideoFrame:(CVImageBufferRef)videoFrame
     withSampleBuffer:(QTSampleBuffer *)sampleBuffer
       fromConnection:(QTCaptureConnection *)connection
{
    CVBufferRetain(videoFrame);
    CVPixelBufferRef pixelBuffer = videoFrame;
    
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    unsigned char *src = (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);
    unsigned char *dest = buffer;
    
    if (color) {
        
        if (crop) {
            unsigned char *src_buf = src + 3*(yoff*cam_width + xoff);
            
            for (int i=0;i<frm_height;i++) {
                memcpy(dest, src_buf, 3*frm_width);
                
                src_buf += 3*cam_width;
                dest += 3*frm_width;
            }
            
        } else memcpy(dest,src,cam_width*cam_height*3);
        
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
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    CVBufferRelease(videoFrame);
    new_frame = true;
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

QTKitCamera::QTKitCamera(CameraConfig *cam_cfg):CameraEngine(cam_cfg)
{
    cam_buffer = NULL;
    running=false;
    disconnected = false;
    lost_frames=0;
    timeout = 1000;
	
	cam_cfg->driver = DRIVER_DEFAULT;
}

QTKitCamera::~QTKitCamera()
{
    [captureSession release];
    [captureDeviceInput release];
    [captureDecompressedVideoOutput release];
    [captureDevice release];
    [grabber release];
}

int QTKitCamera::getDeviceCount() {
	
	NSArray *dev_list0 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
	NSArray *dev_list1 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
	int count =  [dev_list0 count] + [dev_list1 count];
	[dev_list0 release];
	[dev_list1 release];
	return count;
}

std::vector<CameraConfig> QTKitCamera::getCameraConfigs(int dev_id) {
    
    std::vector<CameraConfig> cfg_list;
	
	int dev_count = getDeviceCount();
	if (dev_count==0) return cfg_list;
	
	NSMutableArray *dev_list = [NSMutableArray arrayWithCapacity:dev_count];

    NSArray *dev_list0 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
	for (QTCaptureDevice* dev in dev_list0) [dev_list addObject:dev];
    NSArray *dev_list1 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
	for (QTCaptureDevice* dev in dev_list1) [dev_list addObject:dev];
	[dev_list0 release];
	[dev_list1 release];
    
    int cam_id = -1;
    for (QTCaptureDevice* dev in dev_list) {
		cam_id ++;
		if ((dev_id>=0) && (dev_id!=cam_id)) continue;
		
        CameraConfig cam_cfg;
        CameraTool::initCameraConfig(&cam_cfg);
        
        sprintf(cam_cfg.name,"%s",[[dev localizedDisplayName] cStringUsingEncoding:1]);
        cam_cfg.driver = DRIVER_DEFAULT;
        cam_cfg.device = cam_id;
		
		
		NSArray *formats = [dev formatDescriptions];
		std::cout << "F: " <<[formats count] << std::endl;
		[formats release];
		
        for (QTFormatDescription* fd in [dev formatDescriptions]) {
            
            int32_t codec = [fd formatType];
			if (codec == '420f') codec = '420v';
			
			cam_cfg.cam_format = FORMAT_UNKNOWN;
			for (int i=FORMAT_MAX;i>0;i--) {
				if (codec == codec_table[i]) {
					cam_cfg.cam_format = i;
					break;
				}
			}
			
            NSSize size = [[fd attributeForKey:QTFormatDescriptionVideoEncodedPixelsSizeAttribute] sizeValue];
            cam_cfg.cam_width = (int)size.width;
            cam_cfg.cam_height = (int)size.height;
            cam_cfg.cam_fps = 30;
            
            cfg_list.push_back(cam_cfg);
        }
    }
	
	[dev_list release];
    return cfg_list;
}

CameraEngine* QTKitCamera::getCamera(CameraConfig *cam_cfg) {
	
	int dev_count = getDeviceCount();
	if (dev_count==0) return NULL;
	
	if ((cam_cfg->device==SETTING_MIN) || (cam_cfg->device==SETTING_DEFAULT)) cam_cfg->device=0;
	else if (cam_cfg->device==SETTING_MAX) cam_cfg->device=dev_count-1;

	std::vector<CameraConfig> cfg_list = QTKitCamera::getCameraConfigs(cam_cfg->device);
	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);
	
	if (cam_cfg->force)return new QTKitCamera(cam_cfg);
	
	for (unsigned int i=0;i<cfg_list.size();i++) {
		
		if (cam_cfg->cam_format != cfg_list[i].cam_format) continue;
		if ((cam_cfg->cam_width >=0) && (cam_cfg->cam_width != cfg_list[i].cam_width)) continue;
		if ((cam_cfg->cam_height >=0) && (cam_cfg->cam_height != cfg_list[i].cam_height)) continue;
		if ((cam_cfg->cam_fps >=0) && (cam_cfg->cam_fps != cfg_list[i].cam_fps)) continue;
			
		return new QTKitCamera(cam_cfg);
	}
	
	return NULL;
}

bool QTKitCamera::initCamera() {
    
    NSError *error;
	int dev_count = getDeviceCount();
	if (dev_count==0) return false;
	
	NSMutableArray *dev_list = [NSMutableArray arrayWithCapacity:dev_count];
	NSArray *dev_list0 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
	for (QTCaptureDevice* dev in dev_list0) [dev_list addObject:dev];
	NSArray *dev_list1 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
	for (QTCaptureDevice* dev in dev_list1) [dev_list addObject:dev];
	[dev_list0 release];
	[dev_list1 release];
	
	//Get the capture device
    captureDevice = [dev_list objectAtIndex:cfg->device];
    [dev_list release];
    
	if (!captureDevice) return false;
	if (![captureDevice open:&error]) return false;
    
    //Create the capture session
    captureSession = [[QTCaptureSession alloc] init];
    
    // Add the video device to the session as device input
    captureDeviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:captureDevice];
    bool success = [captureSession addInput:captureDeviceInput error:&error];
    if (!success) return false;
    
    // Add a decompressed video output that returns
    // raw frames to the session
    captureDecompressedVideoOutput =
    [[QTCaptureDecompressedVideoOutput alloc] init];
    
	std::vector<CameraConfig> cfg_list = getCameraConfigs(cfg->device);
	if (cfg->cam_format==FORMAT_UNKNOWN) cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cfg, cfg_list);
    
    int pixelformat = kCVPixelFormatType_422YpCbCr8_yuvs;
    if (cfg->color) pixelformat = kCVPixelFormatType_24RGB;
    
    [captureDecompressedVideoOutput setPixelBufferAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                              [NSNumber numberWithBool:NO], kCVPixelBufferOpenGLCompatibilityKey,
                                                              [NSNumber numberWithFloat:cfg->cam_width], (id)kCVPixelBufferWidthKey,
                                                              [NSNumber numberWithFloat:cfg->cam_height], (id)kCVPixelBufferHeightKey,
                                                              [NSNumber numberWithLong:pixelformat],
															  (id)kCVPixelBufferPixelFormatTypeKey, nil]];
    
    [captureDecompressedVideoOutput setAutomaticallyDropsLateVideoFrames:true];
    
    if ([captureDecompressedVideoOutput respondsToSelector:@selector(setMinimumVideoFrameInterval:)]) {
        [captureDecompressedVideoOutput setMinimumVideoFrameInterval:(1.0f / cfg->cam_fps)];
       // cfg->cam_fps = (int)floor((1.0/[captureDecompressedVideoOutput minimumVideoFrameInterval])+0.5f);
    }
    
    NSDictionary* pixAttr = [captureDecompressedVideoOutput pixelBufferAttributes];
    
    CGSize size;
    if (nil != pixAttr)
        size = CGSizeMake(
                          [[pixAttr valueForKey:(id)kCVPixelBufferWidthKey] floatValue],
                          [[pixAttr valueForKey:(id)kCVPixelBufferHeightKey] floatValue]
                          );
    else {
        NSSize s = [[[[[captureDeviceInput connections] objectAtIndex:0] formatDescription]
                     attributeForKey:QTFormatDescriptionVideoCleanApertureDisplaySizeAttribute] sizeValue];
        size = NSSizeToCGSize(s);
    }
    
    sprintf(cfg->name,"%s",[[captureDevice localizedDisplayName] cStringUsingEncoding:1]);
    
    cfg->cam_width = size.width;
    cfg->cam_height = size.height;
    setupFrame();
    
    if (cfg->frame) {
        grabber = [[FrameGrabber alloc] initWithCropSize:cfg->cam_width :cfg->cam_height :cfg->buf_format :cfg->frame_width :cfg->frame_height :cfg->frame_xoff :cfg->frame_yoff];
    } else {
        grabber = [[FrameGrabber alloc] initWithCameraSize:cfg->cam_width :cfg->cam_height: cfg->buf_format];
    }
    
    [captureDecompressedVideoOutput setDelegate:grabber];
    return [captureSession addOutput:captureDecompressedVideoOutput error:&error];
}

unsigned char* QTKitCamera::getFrame()
{
    unsigned char *frameBuffer = [grabber getFrame];
    if (frameBuffer!=NULL) {
        timeout=100;
        lost_frames=0;
        return frameBuffer;
    } else {
        usleep(10000);
        lost_frames++;
        if (lost_frames>timeout) {
            disconnected=true;
            running=false;
        }
        
        return NULL;		
    }
}

bool QTKitCamera::startCamera()
{
    [captureSession startRunning];
    running = true;
    return true;
}

bool QTKitCamera::stopCamera()
{
    [captureSession stopRunning];
    running=false;
    return true;
}

bool QTKitCamera::stillRunning() {
    return running;
}

bool QTKitCamera::resetCamera()
{
    return (stopCamera() && startCamera());
}

bool QTKitCamera::closeCamera()
{
    
    if (!disconnected) {
        updateSettings();
        CameraTool::saveSettings();
    }
    
    [[captureDeviceInput device] close];
    return true;
}
