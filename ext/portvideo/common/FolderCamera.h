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

#ifndef NDEBUG

#ifndef FOLDERCAMERA_H
#define FOLDERCAMERA_H

#include <stdio.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif
#include "CameraEngine.h"

#include <list>
#include <string>

class FolderCamera : public CameraEngine
{
public:
	FolderCamera(CameraConfig *cam_cfg);
	~FolderCamera();
	
	static CameraEngine* getCamera(CameraConfig *cam_cfg);
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
	std::list<std::string> image_list;
	std::list<std::string>::iterator image_iterator;
	unsigned char *file_buffer;
	
};

#endif
#endif
