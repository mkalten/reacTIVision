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

#ifndef MULTICAMERA_H
#define	MULTICAMERA_H

#include <vector>
#include "CameraEngine.h"
#include "CameraTool.h"
#include "tinyxml2.h"

class MultiCamera : public CameraEngine
{
public:
	MultiCamera(CameraConfig *cam_cfg);
	virtual ~MultiCamera();

    static std::vector<CameraConfig> getCameraConfigs(std::vector<CameraConfig> child_cams);
	static CameraEngine* getCamera(CameraConfig* cam_cfg);

	bool initCamera();
	bool startCamera();
	unsigned char* getFrame();
	bool stopCamera();
	bool stillRunning() { return running; }
	bool resetCamera();
	bool closeCamera();

	void printInfo();

	int getCameraSettingStep(int mode);
	int getCameraSetting(int mode);
	int getMaxCameraSetting(int mode);
	int getMinCameraSetting(int mode);
	bool getCameraSettingAuto(int mode);
	int getDefaultCameraSetting(int mode);
	bool hasCameraSetting(int mode);
	bool hasCameraSettingAuto(int mode);

	bool setCameraSettingAuto(int mode, bool flag);
	bool setCameraSetting(int mode, int value);
	bool setDefaultCameraSetting(int mode);

private:
	std::vector<CameraEngine*> cameras_;
};

#endif	/* MULTICAMERA_H */
