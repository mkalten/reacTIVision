/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>
 
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

#ifndef NDEBUG
#ifndef FileCamera_H
#define FileCamera_H

#include <stdio.h>
#include "CameraEngine.h"

#ifndef WIN32
#include <unistd.h>
#endif

class FileCamera : public CameraEngine
{
public:
	FileCamera(CameraConfig *cam_cfg);
	~FileCamera();

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
    int getDefaultCameraSetting(int mode);
    bool setDefaultCameraSetting(int mode);
	bool setCameraSetting(int mode, int value);
	int getCameraSetting(int mode);
	int getMaxCameraSetting(int mode);
	int getMinCameraSetting(int mode);
	bool hasCameraSetting(int mode);
    bool hasCameraSettingAuto(int mode);
	bool showSettingsDialog(bool lock);

};

#endif
#endif
