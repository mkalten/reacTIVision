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
#include "../common/CameraEngine.h"

#include <vector>
#include <string>

class FolderCamera : public CameraEngine
{
public:
	FolderCamera(const char* cfg);
	~FolderCamera();

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
    void showSettingsDialog();

private:
	std::vector<std::string> image_list;
	std::vector<std::string>::iterator image_iterator;

};

#endif
#endif
