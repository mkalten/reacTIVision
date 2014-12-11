/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2014 Martin Kaltenbrunner <martin@tuio.org>
 
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

#include "../common/cameraEngine.h"
#include "../common/SDLinterface.h"

@interface FrameGrabber : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    bool new_frame;
    int cam_width, cam_height;
    int frm_width, frm_height;
    int xoff, yoff;
    unsigned char *buffer;
    bool crop;
}
- (id) initWithCameraSize:(int)w :(int)h;
- (id) initWithCropSize:(int)cw :(int)ch :(int)fw :(int)fh :(int)xo :(int)yo;
- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection;
- (unsigned char*) getFrame;

@end

class AVfoundationCamera : public CameraEngine
{

public:
	AVfoundationCamera(const char* config_file);
	~AVfoundationCamera();
	
    static void listDevices();

	bool findCamera();
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
	void showSettingsDialog();
    
private:
    
    bool disconnected;
    
    VVUVCController				*uvcController;
    FrameGrabber *grabber;
    
    AVCaptureSession			*session;
    AVCaptureDeviceInput		*videoDeviceInput;
    AVCaptureVideoDataOutput    *videoOutput;
    
    NSMutableArray			*videoDevices;
    AVCaptureDevice         *selectedVideoDevice;
    
    NSArray                 *videoDeviceFormats;
    AVCaptureDeviceFormat   *selectedVideoDeviceFormat;
    
    AVFrameRateRange        *selectedFrameRateRange;
};
#endif
