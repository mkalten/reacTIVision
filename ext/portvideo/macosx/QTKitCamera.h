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
#ifndef QTKitCamera_H
#define QTKitCamera_H

#include "CameraEngine.h"
#import <QTKit/QTKit.h>

#ifndef kCVPixelFormatType_422YpCbCr8_yuvs
#define kCVPixelFormatType_422YpCbCr8_yuvs 2037741171
#endif

static int32_t codec_table[] = { 0, 40,  0,  24, 0, 0, 0, 0, 0, 0, 'yuvs', '2vuy', 0, 'v308', '420v', '410v', 0, 0, 0, 0, 'dmb1', 'dmb1', 'mp1v', 'mp2v', 'mp4v', 'h263', 'avc1', 'dvcp', 'dvc ' };

@interface FrameGrabber : NSObject //: NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
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
- (void)captureOutput:(QTCaptureOutput *)captureOutput
  didOutputVideoFrame:(CVImageBufferRef)videoFrame
	 withSampleBuffer:(QTSampleBuffer *)sampleBuffer
	   fromConnection:(QTCaptureConnection *)connection;
- (unsigned char*) getFrame;

@end

class QTKitCamera : public CameraEngine
{
	
public:
	QTKitCamera(CameraConfig *cam_cfg);
	~QTKitCamera();
	
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
	
	int getCameraSettingStep(int mode) { return 0; }
	bool setCameraSettingAuto(int mode, bool flag) { return false; }
	bool getCameraSettingAuto(int mode) { return false; }
	bool setCameraSetting(int mode, int value) { return false; }
	int getCameraSetting(int mode) { return 0; }
	int getMaxCameraSetting(int mode) { return 0; }
	int getMinCameraSetting(int mode) { return 0; }
	bool setDefaultCameraSetting(int mode) { return false; }
	int getDefaultCameraSetting(int mode) { return 0; }
	bool hasCameraSetting(int mode) { return false; }
	bool hasCameraSettingAuto(int mode) { return false; }
	
	bool showSettingsDialog(bool lock) { return lock; }
	void control(unsigned char key) {};
	
private:
	
	bool disconnected;
	FrameGrabber *grabber;
	
	QTCaptureSession                    *captureSession;
	QTCaptureDeviceInput                *captureDeviceInput;
	QTCaptureDecompressedVideoOutput    *captureDecompressedVideoOutput;
	QTCaptureDevice						*captureDevice;
};
#endif
