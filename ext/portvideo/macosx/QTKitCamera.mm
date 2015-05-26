/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2014 Martin Kaltenbrunner <martin@tuio.org>
 
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
    lost_frames=0;
    timeout = 1000;
}

QTKitCamera::~QTKitCamera()
{
    [captureSession release];
    [captureDeviceInput release];
    [captureDecompressedVideoOutput release];
    [captureDevice release];
    [grabber release];
}

std::vector<CameraConfig> QTKitCamera::findDevices() {
    
    std::vector<CameraConfig> cfg_list;
    
    NSArray *dev_list0 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    NSArray *dev_list1 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
    int capacity = [dev_list0 count] + [dev_list1 count];
    if (capacity==0) return cfg_list;
    
    NSMutableArray *dev_list = [NSMutableArray arrayWithCapacity:capacity];
    
    for (QTCaptureDevice* dev in dev_list0) {
        [dev_list addObject:dev];
    }
    
    for (QTCaptureDevice* dev in dev_list1) {
        [dev_list addObject:dev];
    }
    
    int count = [dev_list count];
    if(count==0) {
        printf("no QTKit camera found\n");
        return cfg_list;
    } else if (count==1) printf("1 QTKit camera found:\n");
    else printf("%d QTKit cameras found:\n",count);
    
    unsigned int i = 0;
    for (QTCaptureDevice* dev in dev_list) {
        
        CameraConfig cam_cfg;
        CameraTool::initCameraConfig(&cam_cfg);
        
        sprintf(cam_cfg.name,"%s",[[dev localizedDisplayName] cStringUsingEncoding:1]);
        cam_cfg.driver = DRIVER_DEFAULT;
        cam_cfg.device = i;
        
        for (QTFormatDescription* fd in [dev formatDescriptions]) {
            
            int32_t codec = [fd formatType];
            if ((codec=='yuvs') || (codec=='2vuy'))
                cam_cfg.cam_format = FORMAT_YUYV;
            else if ((codec=='420v') || (codec=='420f'))
                cam_cfg.cam_format = FORMAT_420P;
            else if ((codec=='jpeg') || (codec=='dmb1'))
                cam_cfg.cam_format = FORMAT_JPEG;
            else if (codec=='avc1')
                cam_cfg.cam_format = FORMAT_H264;
            else if (codec=='h263')
                cam_cfg.cam_format = FORMAT_H263;
            else if ((codec=='mp4v') || (codec=='mp2v') || (codec=='mp1v'))
                cam_cfg.cam_format = FORMAT_MPEG;
            else if ((codec=='dvc ') || (codec=='dvcp'))
                cam_cfg.cam_format = FORMAT_DV;
            else if (codec==40)
                cam_cfg.cam_format = FORMAT_GRAY; // probably incorrect workaround
            else
                cam_cfg.cam_format = FORMAT_UNKNOWN;
            
            NSSize size = [[fd attributeForKey:QTFormatDescriptionVideoEncodedPixelsSizeAttribute] sizeValue];
            cam_cfg.cam_width = (int)size.width;
            cam_cfg.cam_height = (int)size.height;
            cam_cfg.cam_fps = SETTING_MAX;
            
            cfg_list.push_back(cam_cfg);
        }
        i++;
    }
    
    return cfg_list;
}

void QTKitCamera::listDevices() {
    
    NSArray *dev_list0 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    NSArray *dev_list1 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
    int capacity = [dev_list0 count] + [dev_list1 count];
    if (capacity==0) return;
    
    NSMutableArray *dev_list = [NSMutableArray arrayWithCapacity:capacity];
    
    for (QTCaptureDevice* dev in dev_list0) {
        [dev_list addObject:dev];
    }
    
    for (QTCaptureDevice* dev in dev_list1) {
        [dev_list addObject:dev];
    }
    
    int count = [dev_list count];
    if(count==0) {
        printf("no QTKit camera found\n");
        return;
    } else if (count==1) printf("1 QTKit camera found:\n");
    else printf("%d QTKit cameras found:\n",count);
    
    unsigned int i = 0;
    for (QTCaptureDevice* dev in dev_list) {
        printf("#%d: %s\n",i,[[dev localizedDisplayName] cStringUsingEncoding:1]);
        i++;
        
        for (QTFormatDescription* fd in [dev formatDescriptions]) {
            //printf("\t%s\n",[[fd localizedFormatSummary] cStringUsingEncoding:NSUTF8StringEncoding ]);
            
            int32_t codec = [fd formatType];
            if ((codec=='yuvs') || (codec=='2vuy')) printf("\t\tformat: YUYV (%s)\n",FourCC2Str(codec));
            else if ((codec=='420v') || (codec=='420f')) printf("\t\tformat: YUV420 (%s)\n",FourCC2Str(codec));
            else if ((codec=='jpeg') || (codec=='dmb1')) printf("\t\tformat: JPEG (%s)\n",FourCC2Str(codec));
            else if (codec=='avc1') printf("\t\tformat: H.264 (%s)\n",FourCC2Str(codec));
            else if (codec=='h263') printf("\t\tformat: H.263 (%s)\n",FourCC2Str(codec));
            else if ((codec=='mp4v') || (codec=='mp2v') || (codec=='mp1v')) printf("\t\tformat: MPEG (%s)\n",FourCC2Str(codec));
            else if ((codec=='dvc ') || (codec=='dvcp')) printf("\t\tformat: DVC (%s)\n",FourCC2Str(codec));
            else if (codec==40)printf("\t\tformat: MONO8\n"); // probably incorrect workaround
            else printf("\t\tformat: other (%s)\n",FourCC2Str(codec));
            
            NSSize size = [[fd attributeForKey:QTFormatDescriptionVideoEncodedPixelsSizeAttribute] sizeValue];
            printf("\t\t\t%dx%d\n",(int)size.width,(int)size.height);
        }
    }
}


bool QTKitCamera::findCamera() {
    
    //Create the capture session
    captureSession = [[QTCaptureSession alloc] init];
    
    NSError *error;
    NSArray *dev_list0 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    NSArray *dev_list1 = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
    int capacity = [dev_list0 count] + [dev_list1 count];
    if (capacity==0) return false;
    
    NSMutableArray *dev_list = [NSMutableArray arrayWithCapacity:capacity];
    
    for (QTCaptureDevice* dev in dev_list0) {
        [dev_list addObject:dev];
    }
    
    for (QTCaptureDevice* dev in dev_list1) {
        [dev_list addObject:dev];
    }
    
    if (cfg->device<0) cfg->device=0;
    if (cfg->device>=[dev_list count]) cfg->device = [dev_list count]-1;
    
    captureDevice = [dev_list objectAtIndex:cfg->device];
    [dev_list release];
    
    if(!captureDevice) {
        captureDevice= [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo];
        cfg->device = 0;
    }
    
    if (!captureDevice) {
        captureDevice = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeMuxed];
        cfg->device = 0;
    }
    
    if (captureDevice) {
        if (![captureDevice open:&error]) {
            [captureSession release];
            printf("no QTKit cameras found\n");
            return false;
        }
    } else {
        [captureSession release];
        printf("no QTKit cameras found\n");
        return false;
    }
    
    if (cfg->device>=0) return true;
    else {
        printf("no QTKit cameras found\n");
        return false;
    }
}

bool QTKitCamera::initCamera() {
    
    NSError *error;
    
    if (!captureDevice) return false;
    
    // Add the video device to the session as device input
    captureDeviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:captureDevice];
    bool success = [captureSession addInput:captureDeviceInput error:&error];
    if (!success) return false;
    
    // Add a decompressed video output that returns
    // raw frames to the session
    captureDecompressedVideoOutput =
    [[QTCaptureDecompressedVideoOutput alloc] init];
    
    if (cfg->cam_width==SETTING_MAX) cfg->cam_width = 640;
    if (cfg->cam_height==SETTING_MAX) cfg->cam_height = 480;
    if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps = 30;
    
    int pixelformat = kCVPixelFormatType_422YpCbCr8_yuvs;
    if (cfg->color) pixelformat = kCVPixelFormatType_24RGB;
    
    [captureDecompressedVideoOutput setPixelBufferAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                              [NSNumber numberWithBool:YES], kCVPixelBufferOpenGLCompatibilityKey,
                                                              [NSNumber numberWithFloat:cfg->cam_width], (id)kCVPixelBufferWidthKey,
                                                              [NSNumber numberWithFloat:cfg->cam_height], (id)kCVPixelBufferHeightKey,
                                                              [NSNumber numberWithLong:pixelformat], (id)kCVPixelBufferPixelFormatTypeKey,
                                                              nil]];
    
    [captureDecompressedVideoOutput setAutomaticallyDropsLateVideoFrames:true];
    
    if ([captureDecompressedVideoOutput respondsToSelector:@selector(setMinimumVideoFrameInterval:)]) {
        [captureDecompressedVideoOutput setMinimumVideoFrameInterval:(1.0f / cfg->cam_fps)];
        cfg->cam_fps = (int)floor((1.0/[captureDecompressedVideoOutput minimumVideoFrameInterval])+0.5f);
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
    success = [captureSession addOutput:captureDecompressedVideoOutput error:&error];
    if (!success) return false;
    
    
    return true;
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
        if (lost_frames>timeout) running=false;
        
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
    [[captureDeviceInput device] close];
    return true;
}