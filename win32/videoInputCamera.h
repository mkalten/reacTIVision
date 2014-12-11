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

#ifndef videoInputCamera_H
#define videoInputCamera_H

#include <videoInput.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/CameraEngine.h"

#define hibyte(x) (unsigned char)((x)>>8)

class videoInputCamera : public CameraEngine
{

public:
	videoInputCamera(const char* config_file);
	~videoInputCamera();

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
	int getMinCameraSetting(int mode);
	int getMaxCameraSetting(int mode);
	int getCameraSetting(int mode);
	bool setCameraSetting(int mode, int value);
	bool setCameraSettingAuto(int mode, bool flag);
	bool getCameraSettingAuto(int mode);
	bool setDefaultCameraSetting(int mode);
	int getDefaultCameraSetting(int mode);
	void updateSettings();

	void control(int key);
	void showSettingsDialog();

private:

	bool disconnect;
	videoInput *VI;
};

#endif
