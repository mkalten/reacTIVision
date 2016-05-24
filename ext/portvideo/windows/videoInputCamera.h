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

#ifndef videoInputCamera_H
#define videoInputCamera_H

#include <videoInput.h>
#include <stdio.h>
#include <stdlib.h>
#include "CameraEngine.h"

#define FLAGS_AUTO        0X0001L
#define FLAGS_MANUAL      0X0002L

static int comCount = 0;

class videoInputCamera : public CameraEngine
{

public:
	videoInputCamera(CameraConfig *cam_cfg);
	~videoInputCamera();

	static std::vector<CameraConfig> getCameraConfigs(int dev_id=-1);
	static int getDeviceCount();
	static CameraEngine* getCamera(CameraConfig *cam_cfg);

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
	bool hasCameraSetting(int mode);
    bool hasCameraSettingAuto(int mode);

	void control(int key);
	bool showSettingsDialog(bool lock);

private:

	bool disconnect;
	videoInput *VI;

	static bool comInit();
	static bool comUnInit();
	static HRESULT getDevice(IBaseFilter **pSrcFilter, int deviceID, WCHAR * wDeviceName, char * nDeviceName);
	static HRESULT getDevice(IBaseFilter **pSrcFilter, int deviceID);

	bool videoInputCamera::getVideoSettingValue(int deviceID, long Property, long &currentValue, long &flags);
	bool videoInputCamera::setVideoSettingValue(int deviceID, long Property, long lValue, long Flags);
	bool videoInputCamera::setVideoSettingDefault(int deviceID, long Property);
	bool videoInputCamera::getVideoSettingRange(int deviceID, long Property, long &min, long &max, long &SteppingDelta, long &flags, long &defaultValue);

	bool videoInputCamera::getVideoControlValue(int deviceID, long Property, long &currentValue, long &flags);
	bool videoInputCamera::setVideoControlValue(int deviceID, long Property, long lValue, long Flags);
	bool videoInputCamera::setVideoControlDefault(int deviceID, long Property);
	bool videoInputCamera::getVideoControlRange(int deviceID, long Property, long &min, long &max, long &SteppingDelta, long &flags, long &defaultValue);

	static void __cdecl settingsThread(void * objPtr);
	static HRESULT ShowFilterPropertyPages(IBaseFilter *pFilter);

	static GUID getMediaSubtype(int type);
	static int getMediaSubtype(GUID type);
	static void makeGUID( GUID *guid, unsigned long Data1, unsigned short Data2, unsigned short Data3, unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3,
	unsigned char b4, unsigned char b5, unsigned char b6, unsigned char b7 );

	static void deleteMediaType(AM_MEDIA_TYPE *pmt);
};

#endif
