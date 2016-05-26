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

#pragma comment(lib,"Strmiids.lib") 

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <wchar.h>
#include <string>
#include <vector>

//this is for TryEnterCriticalSection
#ifndef _WIN32_WINNT
	#   define _WIN32_WINNT 0x501
#endif
#include <windows.h>

//#include <videoInput.h>
#include <stdio.h>
#include <stdlib.h>
#include "CameraEngine.h"

//allows us to directShow classes here with the includes in the cpp
struct ICaptureGraphBuilder2;
struct IGraphBuilder;
struct IBaseFilter;
struct IAMCrossbar;
struct IMediaControl;
struct ISampleGrabber;
struct IMediaEventEx;
struct IAMStreamConfig;
struct _AMMediaType;
class SampleGrabberCallback;
typedef _AMMediaType AM_MEDIA_TYPE;


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
	//videoInput *VI;

	static bool comInit();
	static bool comUnInit();
	static HRESULT getDevice(IBaseFilter **pSrcFilter, int deviceID, WCHAR * wDeviceName, char * nDeviceName);
	static HRESULT getDevice(IBaseFilter **pSrcFilter, int deviceID);

	bool getVideoSettingValue(int deviceID, long prop, long &value, long &flag);
	bool setVideoSettingValue(int deviceID, long prop, long value, long flag);
	bool getVideoSettingRange(int deviceID, long prop, long &min, long &max, long &step, long &flag, long &dflt);

	bool getVideoControlValue(int deviceID, long prop, long &value, long &flag);
	bool setVideoControlValue(int deviceID, long prop, long value, long flag);
	bool getVideoControlRange(int deviceID, long prop, long &min, long &max, long &step, long &flag, long &dflt);

	static void __cdecl settingsThread(void * objPtr);
	static HRESULT ShowFilterPropertyPages(IBaseFilter *pFilter);

	static GUID getMediaSubtype(int type);
	static int getMediaSubtype(GUID type);
	static void makeGUID( GUID *guid, unsigned long Data1, unsigned short Data2, unsigned short Data3, unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3,
	unsigned char b4, unsigned char b5, unsigned char b6, unsigned char b7 );

	static void deleteMediaType(AM_MEDIA_TYPE *pmt);
	void NukeDownstream(IBaseFilter *pBF);
	void destroyGraph();

	//HRESULT routeCrossbar(ICaptureGraphBuilder2 **ppBuild, IBaseFilter **pVidInFilter, int conType, GUID captureMode);
	HRESULT setupDevice();
	HRESULT stopDevice();
	bool setSizeAndSubtype();


	ICaptureGraphBuilder2 *pCaptureGraph;	// Capture graph builder object
	IGraphBuilder *pGraph;					// Graph builder object
	IMediaControl *pControl;				// Media control object
	IBaseFilter *pVideoInputFilter;  		// Video Capture filter
	IBaseFilter *pGrabberF;
	IBaseFilter * pDestFilter;
	IAMStreamConfig *streamConf;
	ISampleGrabber * pGrabber;    			// Grabs frame
	AM_MEDIA_TYPE * pAmMediaType;
	IMediaEventEx * pMediaEvent;
	SampleGrabberCallback * sgCallback;

	//unsigned char *pixels;
	//char *pBuffer;

	bool isFrameNew();
	//unsigned char* videoInputCamera::getPixelPointer();

};

#endif
