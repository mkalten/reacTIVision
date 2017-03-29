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

#ifndef AVfoundationCamera_H
#define AVfoundationCamera_H

#import <AVFoundation/AVFoundation.h>
#import <Cocoa/Cocoa.h>
#import <VVUVCKit/VVUVCKit.h>

#include "CameraEngine.h"

static int32_t codec_table[] = { 0, 40,  0,  24, 0, 0, 0, 0, 0, 0, 'yuvs', '2vuy', 0, 'v308', '420v', '410v', 0, 0, 0, 0, 'dmb1', 'dmb1', 'mp1v', 'mp2v', 'mp4v', 'h263', 'avc1', 'dvcp', 'dvc ' };


@interface FrameGrabber : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    bool new_frame;
    int cam_width, cam_height;
    int frm_width, frm_height;
    int xoff, yoff;
    unsigned char *buffer;
    bool crop;
    bool color;
}
- (id) initWithCameraSize:(int)w :(int)h :(int)b;
- (id) initWithCropSize:(int)cw :(int)ch :(int)b :(int)fw :(int)fh :(int)xo :(int)yo;
- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection;
- (unsigned char*) getFrame;

@end

class AVfoundationCamera : public CameraEngine
{

public:
    AVfoundationCamera(CameraConfig* cam_cfg);
    ~AVfoundationCamera();
	
	static int getDeviceCount();
	static std::vector<CameraConfig> getCameraConfigs(int dev_id = -1);
	static CameraEngine* getCamera(CameraConfig* cam_cfg);
	
	bool initCamera();
	bool startCamera();
	unsigned char* getFrame();
	bool stopCamera();
	bool stillRunning();
	bool resetCamera();
	bool closeCamera();

    int getCameraSettingStep(int mode);
    bool setCameraSettingAuto(int mode, bool flag);
    bool getCameraSettingAuto(int mode);
    bool setCameraSetting(int mode, int value);
    int getCameraSetting(int mode);
    int getMaxCameraSetting(int mode);
    int getMinCameraSetting(int mode);
    bool setDefaultCameraSetting(int mode);
    int getDefaultCameraSetting(int mode);
    bool hasCameraSetting(int mode);
    bool hasCameraSettingAuto(int mode);
    
	bool showSettingsDialog(bool lock);
    
private:
    
    bool disconnected;
    
    VVUVCController				*uvcController;
    FrameGrabber                *grabber;
    
    AVCaptureSession			*session;
    AVCaptureDeviceInput		*videoDeviceInput;
    AVCaptureVideoDataOutput    *videoOutput;
    AVCaptureDevice             *videoDevice;
};
#endif
