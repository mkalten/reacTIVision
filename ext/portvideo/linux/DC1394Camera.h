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

#ifndef DC1394Camera_H
#define DC1394Camera_H

#include <unistd.h>
#include "CameraEngine.h"
#include <dc1394/dc1394.h>
	
class DC1394Camera : public CameraEngine
{
public:
    DC1394Camera(CameraConfig* cam_cfg);
	~DC1394Camera();
	
	static int getDeviceCount();
	static std::vector<CameraConfig> getCameraConfigs(int dev_id=-1);
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
    int getDefaultCameraSetting(int mode);
    bool setDefaultCameraSetting(int mode);
    bool hasCameraSetting(int mode);
    bool hasCameraSettingAuto(int mode);

private:
	dc1394_t *d;
	dc1394camera_list_t *list;
	dc1394camera_t *camera;
	dc1394color_coding_t coding;
};

#endif
