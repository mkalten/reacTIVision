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

#include "videoInputCamera.h"
#include "CameraTool.h"
#include <dshow.h>

videoInputCamera::videoInputCamera(CameraConfig *cam_cfg):CameraEngine (cam_cfg)
{
	VI = new videoInput();
	VI->setVerbose(false);
	cam_buffer = NULL;
	running = false;

	timeout= 2000;
	lost_frames=0;
	disconnect = false;
}

videoInputCamera::~videoInputCamera()
{
	delete VI;
	if (cam_buffer!=NULL) delete cam_buffer;
}

void videoInputCamera::deleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL) {
		if (pmt->cbFormat != 0) {
			CoTaskMemFree((PVOID)pmt->pbFormat);
			pmt->cbFormat = 0;
			pmt->pbFormat = NULL;
		}
		if (pmt->pUnk != NULL) {
			// Unecessary because pUnk should not be used, but safest.
			pmt->pUnk->Release();
			pmt->pUnk = NULL;
		}

		CoTaskMemFree(pmt);
	}
}

bool videoInputCamera::comInit(){
	HRESULT hr = NULL;

	//no need for us to start com more than once
	if(comCount == 0 ){

		hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
		//hr = CoInitialize(NULL);

		if( hr == RPC_E_CHANGED_MODE){
			//printf("SETUP - COM already setup - threaded VI might not be possible\n");
		}
	}

	comCount++;
	return true;
}

bool videoInputCamera::comUnInit(){
	if(comCount > 0)comCount--;		//decrease the count of instances using com

	if(comCount == 0){
		CoUninitialize();	//if there are no instances left - uninitialize com
		return true;
	}

	return false;
}

HRESULT videoInputCamera::getDevice(IBaseFilter** gottaFilter, int deviceId, WCHAR * wDeviceName, char * nDeviceName){
	BOOL done = false;
	int deviceCounter = 0;

	// Create the System Device Enumerator.
	ICreateDevEnum *pSysDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if (FAILED(hr))
	{
		return hr;
	}

	// Obtain a class enumerator for the video input category.
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

	if (hr == S_OK)
	{
		// Enumerate the monikers.
		IMoniker *pMoniker = NULL;
		ULONG cFetched;
		while ((pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) && (!done))
		{
			if(deviceCounter == deviceId)
			{
				// Bind the first moniker to an object
				IPropertyBag *pPropBag;
				hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
				if (SUCCEEDED(hr))
				{
					// To retrieve the filter's friendly name, do the following:
					VARIANT varName;
					VariantInit(&varName);
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);
					if (SUCCEEDED(hr))
					{

						//copy the name to nDeviceName & wDeviceName
						int count = 0;
						while( varName.bstrVal[count] != 0x00 ) {
							wDeviceName[count] = varName.bstrVal[count];
							nDeviceName[count] = (char)varName.bstrVal[count];
							count++;
						}

						// We found it, so send it back to the caller
						hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)gottaFilter);
						done = true;
					}
					VariantClear(&varName);
					pPropBag->Release();
					pPropBag = NULL;
					pMoniker->Release();
					pMoniker = NULL;
				}
			}
			deviceCounter++;
		}
		pEnumCat->Release();
		pEnumCat = NULL;
	}
	pSysDevEnum->Release();
	pSysDevEnum = NULL;

	if (done) {
		return hr;	// found it, return native error
	} else {
		return VFW_E_NOT_FOUND;	// didn't find it error
	}
}


int videoInputCamera::getDeviceCount() {

	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	int deviceCount = 0;

	comInit();

	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
		reinterpret_cast<void**>(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the video capture category.
		hr = pDevEnum->CreateClassEnumerator(
			CLSID_VideoInputDeviceCategory,
			&pEnum, 0);

		if(hr == S_OK) {
			IMoniker *pMoniker = NULL;

			while (pEnum->Next(1, &pMoniker, NULL) == S_OK){

				pMoniker->Release();
				pMoniker = NULL;

				deviceCount++;
			}

			pDevEnum->Release();
			pDevEnum = NULL;

			pEnum->Release();
			pEnum = NULL;
		}
	}

	comUnInit();
	return deviceCount;
}

void videoInputCamera::makeGUID( GUID *guid, unsigned long Data1, unsigned short Data2, unsigned short Data3,
								unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3,
								unsigned char b4, unsigned char b5, unsigned char b6, unsigned char b7 ){
									guid->Data1 = Data1;
									guid->Data2 = Data2;
									guid->Data3 = Data3;
									guid->Data4[0] = b0; guid->Data4[1] = b1; guid->Data4[2] = b2; guid->Data4[3] = b3;
									guid->Data4[4] = b4; guid->Data4[5] = b5; guid->Data4[6] = b6; guid->Data4[7] = b7;
}


GUID videoInputCamera::getMediaSubtype(int type){

	int count = getDeviceCount();
	comInit();

	HRESULT hr;
	ICaptureGraphBuilder2 *pCaptureGraph;
	IGraphBuilder *pGraph;
	IBaseFilter *pVideoInputFilter;
	IAMStreamConfig *pStreamConf;
	GUID mediaSubType = MEDIASUBTYPE_NULL;

	char 	nDeviceName[255];
	WCHAR 	wDeviceName[255];

	for (int cam_id=0;cam_id<count;cam_id++) {

		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&pCaptureGraph);
		if (FAILED(hr))	// FAILED is a macro that tests the return value
		{
			printf("ERROR - Could not create the Filter Graph Manager\n");
			comUnInit();
			return MEDIASUBTYPE_NULL;
		}

		// Create the Filter Graph Manager.
		hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,IID_IGraphBuilder, (void**)&pGraph);
		if (FAILED(hr))
		{
			printf("ERROR - Could not add the graph builder!\n");
			pCaptureGraph->Release();
			comUnInit();
			return MEDIASUBTYPE_NULL;
		}

		hr = pCaptureGraph->SetFiltergraph(pGraph);
		if (FAILED(hr))
		{
			printf("ERROR - Could not set filtergraph\n");
			pGraph->Release();
			pCaptureGraph->Release();
			comUnInit();
			return MEDIASUBTYPE_NULL;
		}

		memset(wDeviceName, 0, sizeof(WCHAR) * 255);
		memset(nDeviceName, 0, sizeof(char) * 255);
		hr = getDevice(&pVideoInputFilter, cam_id, wDeviceName, nDeviceName);

		if (SUCCEEDED(hr)){
			hr = pGraph->AddFilter(pVideoInputFilter, wDeviceName);
		}else{
			printf("ERROR - Could not find specified video device\n");
			pGraph->Release();
			pCaptureGraph->Release();
			comUnInit();
			return MEDIASUBTYPE_NULL;
		}

		hr = pCaptureGraph->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoInputFilter, IID_IAMStreamConfig, (void **)&pStreamConf);
		if(FAILED(hr)){
			printf("ERROR: Couldn't config the stream!\n");
			pVideoInputFilter->Release();
			pGraph->Release();
			pCaptureGraph->Release();
			comUnInit();
			return MEDIASUBTYPE_NULL;
		}

		int iCount = 0;
		int iSize = 0;
		hr = pStreamConf->GetNumberOfCapabilities(&iCount, &iSize);
		std::vector<CameraConfig> fmt_list;

		if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
		{
			GUID lastFormat = MEDIASUBTYPE_None;
			for (int iFormat = 0; iFormat < iCount; iFormat+=2)
			{
				VIDEO_STREAM_CONFIG_CAPS scc;
				AM_MEDIA_TYPE *pmtConfig;
				hr =  pStreamConf->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
				if (SUCCEEDED(hr)){

					if ( pmtConfig->subtype != lastFormat) {

						if (type == getMediaSubtype(pmtConfig->subtype)) {

							GUID mediaSubtype = pmtConfig->subtype;

							deleteMediaType(pmtConfig);
							pStreamConf->Release();
							pVideoInputFilter->Release();
							pGraph->Release();
							pCaptureGraph->Release();
							comUnInit();

							return mediaSubtype;
						}

						lastFormat = pmtConfig->subtype;
					}

					deleteMediaType(pmtConfig);
				}
			}
		}

		pStreamConf->Release();
		pVideoInputFilter->Release();
		pGraph->Release();
		pCaptureGraph->Release();

	}

	comUnInit();
	return MEDIASUBTYPE_NULL;

}

int videoInputCamera::getMediaSubtype(GUID type){

	GUID MEDIASUBTYPE_Y800, MEDIASUBTYPE_Y8, MEDIASUBTYPE_GREY;
	makeGUID( &MEDIASUBTYPE_Y800, 0x30303859, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 );
	makeGUID( &MEDIASUBTYPE_Y8, 0x20203859, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 );
	makeGUID( &MEDIASUBTYPE_GREY, 0x59455247, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 );

	if ( type == MEDIASUBTYPE_RGB24) return FORMAT_RGB;
	else if(type == MEDIASUBTYPE_RGB32) return FORMAT_RGBA;
	else if(type == MEDIASUBTYPE_RGB555) return 0;
	else if(type == MEDIASUBTYPE_RGB565) return 0;
	else if(type == MEDIASUBTYPE_YUY2) return FORMAT_UYVY;
	else if(type == MEDIASUBTYPE_YVYU) return FORMAT_YVYU;
	else if(type == MEDIASUBTYPE_YUYV) return FORMAT_YUYV;
	else if(type == MEDIASUBTYPE_IYUV) return FORMAT_420P;
	else if(type == MEDIASUBTYPE_UYVY) return FORMAT_UYVY;
	else if(type == MEDIASUBTYPE_YV12) return 0;
	else if(type == MEDIASUBTYPE_YVU9) return 0;
	else if(type == MEDIASUBTYPE_Y411) return FORMAT_YUV411;
	else if(type == MEDIASUBTYPE_Y41P) return FORMAT_410P;
	else if(type == MEDIASUBTYPE_Y211) return FORMAT_YUV211;
	else if(type == MEDIASUBTYPE_AYUV) return FORMAT_YUV444;
	else if(type == MEDIASUBTYPE_Y800) return FORMAT_GRAY;
	else if(type == MEDIASUBTYPE_Y8)   return FORMAT_GRAY;
	else if(type == MEDIASUBTYPE_GREY) return FORMAT_GRAY;
	else if(type == MEDIASUBTYPE_MJPG) return FORMAT_MJPEG;
	else if(type == MEDIASUBTYPE_H264) return FORMAT_H264;
	else return 0;
}

std::vector<CameraConfig> videoInputCamera::getCameraConfigs(int dev_id) {

	std::vector<CameraConfig> cfg_list;

	int count = getDeviceCount();
	if (count==0) return cfg_list;

	comInit();

	HRESULT hr;
	ICaptureGraphBuilder2 *pCaptureGraph;
	IGraphBuilder *pGraph;
	IBaseFilter *pVideoInputFilter;
	IAMStreamConfig *pStreamConf;

	char 	nDeviceName[255];
	WCHAR 	wDeviceName[255];

	for (int cam_id=0;cam_id<count;cam_id++) {
		if ((dev_id>=0) && (dev_id!=cam_id)) continue;
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&pCaptureGraph);
		if (FAILED(hr))	// FAILED is a macro that tests the return value
		{
			printf("ERROR - Could not create the Filter Graph Manager\n");
			comUnInit();
			return cfg_list;
		}

		// Create the Filter Graph Manager.
		hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,IID_IGraphBuilder, (void**)&pGraph);
		if (FAILED(hr))
		{
			printf("ERROR - Could not add the graph builder!\n");
			pCaptureGraph->Release();
			comUnInit();
			return cfg_list;
		}

		hr = pCaptureGraph->SetFiltergraph(pGraph);
		if (FAILED(hr))
		{
			printf("ERROR - Could not set filtergraph\n");
			pGraph->Release();
			pCaptureGraph->Release();
			comUnInit();
			return cfg_list;
		}

		memset(wDeviceName, 0, sizeof(WCHAR) * 255);
		memset(nDeviceName, 0, sizeof(char) * 255);
		hr = getDevice(&pVideoInputFilter, cam_id, wDeviceName, nDeviceName);

		if (SUCCEEDED(hr)){
			hr = pGraph->AddFilter(pVideoInputFilter, wDeviceName);
		}else{
			printf("ERROR - Could not find specified video device\n");
			pGraph->Release();
			pCaptureGraph->Release();
			comUnInit();
			return cfg_list;
		}

		hr = pCaptureGraph->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoInputFilter, IID_IAMStreamConfig, (void **)&pStreamConf);
		if(FAILED(hr)){
			printf("ERROR: Couldn't config the stream!\n");
			pVideoInputFilter->Release();
			pGraph->Release();
			pCaptureGraph->Release();
			comUnInit();
			return cfg_list;
		}

		CameraConfig cam_cfg;
		CameraTool::initCameraConfig(&cam_cfg);

		cam_cfg.driver = DRIVER_DEFAULT;
		cam_cfg.device = cam_id;
		sprintf(cam_cfg.name, "%s", nDeviceName);

		int iCount = 0;
		int iSize = 0;
		hr = pStreamConf->GetNumberOfCapabilities(&iCount, &iSize);
		std::vector<CameraConfig> fmt_list;

		if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
		{
			GUID lastFormat = MEDIASUBTYPE_None;
			for (int iFormat = 0; iFormat < iCount; iFormat+=2)
			{
				VIDEO_STREAM_CONFIG_CAPS scc;
				AM_MEDIA_TYPE *pmtConfig;
				hr =  pStreamConf->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
				if (SUCCEEDED(hr)){

					if ( pmtConfig->subtype != lastFormat) {

						if (fmt_list.size()>0) {
							std::sort(fmt_list.begin(), fmt_list.end());
							cfg_list.insert( cfg_list.end(), fmt_list.begin(), fmt_list.end() );
							fmt_list.clear();
						}
						cam_cfg.cam_format = getMediaSubtype(pmtConfig->subtype);
						lastFormat = pmtConfig->subtype;
					}

					int stepX = scc.OutputGranularityX;
					int stepY = scc.OutputGranularityY;
					if(stepX < 1 || stepY < 1) continue;

					else if ((stepX==1) && (stepY==1)) {

						cam_cfg.cam_width = scc.InputSize.cx;
						cam_cfg.cam_height = scc.InputSize.cy;

						int maxFrameInterval = scc.MaxFrameInterval;
						if (maxFrameInterval==0) maxFrameInterval = 10000000;
						float last_fps=-1;
						VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmtConfig->pbFormat;
						for (int iv=scc.MinFrameInterval;iv<=maxFrameInterval;iv=iv*2) {
							pVih->AvgTimePerFrame = iv;
							hr = pStreamConf->SetFormat(pmtConfig);
							if (hr==S_OK) { hr = pStreamConf->GetFormat(&pmtConfig);
							float fps = ((int)floor(100000000.0f/(float)pVih->AvgTimePerFrame))/10.0f;
							if (fps!=last_fps) {
								cam_cfg.cam_fps = fps;
								fmt_list.push_back(cam_cfg);
								last_fps=fps;
							} }
						}

					} else {
						int x,y;
						for (x=scc.MinOutputSize.cx,y=scc.MinOutputSize.cy;x<=scc.MaxOutputSize.cx,y<=scc.MaxOutputSize.cy;x+=stepX,y+=stepY) {

							cam_cfg.cam_width = x;
							cam_cfg.cam_height = y;

							int maxFrameInterval = scc.MaxFrameInterval;
							if (maxFrameInterval==0) maxFrameInterval = 10000000;
							float last_fps=-1;
							VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmtConfig->pbFormat;
							for (int iv=scc.MinFrameInterval;iv<=maxFrameInterval;iv=iv*2) {
								pVih->AvgTimePerFrame = iv;
								hr = pStreamConf->SetFormat(pmtConfig);
								if (hr==S_OK) { hr = pStreamConf->GetFormat(&pmtConfig);
								float fps = ((int)floor(100000000.0f/(float)pVih->AvgTimePerFrame))/10.0f;
								if (fps!=last_fps) {
									cam_cfg.cam_fps = fps;
									fmt_list.push_back(cam_cfg);
									last_fps=fps;
								} }
							}

						}
					}

					deleteMediaType(pmtConfig);
				}
			}
		}

		if (fmt_list.size()>0) {
			std::sort(fmt_list.begin(), fmt_list.end());
			cfg_list.insert( cfg_list.end(), fmt_list.begin(), fmt_list.end() );
			fmt_list.clear();
		}

		pStreamConf->Release();
		pVideoInputFilter->Release();
		pGraph->Release();
		pCaptureGraph->Release();

	}

	comUnInit();
	return cfg_list;
}

CameraEngine* videoInputCamera::getCamera(CameraConfig *cam_cfg) {

	int cam_count = getDeviceCount();
	if (cam_count==0) return NULL;

	if ((cam_cfg->device==SETTING_MIN) || (cam_cfg->device==SETTING_DEFAULT)) cam_cfg->device=0;
	else if (cam_cfg->device==SETTING_MAX) cam_cfg->device=cam_count-1;

	std::vector<CameraConfig> cfg_list = videoInputCamera::getCameraConfigs(cam_cfg->device);
	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);

	if (cam_cfg->force) return new videoInputCamera(cam_cfg);

	for (unsigned int i=0;i<cfg_list.size();i++) {

		if (cam_cfg->cam_format != cfg_list[i].cam_format) continue;
		if ((cam_cfg->cam_width >=0) && (cam_cfg->cam_width != cfg_list[i].cam_width)) continue;
		if ((cam_cfg->cam_height >=0) && (cam_cfg->cam_height != cfg_list[i].cam_height)) continue;
		if ((cam_cfg->cam_fps >=0) && (cam_cfg->cam_fps != cfg_list[i].cam_fps)) continue;

		return new videoInputCamera(cam_cfg);
	}

	return NULL;
}

bool videoInputCamera::initCamera() {

	int dev_count = getDeviceCount();
	if (dev_count==0) return false;
	if ((cfg->device==SETTING_MIN) || (cfg->device==SETTING_DEFAULT)) cfg->device=0;
	else if (cfg->device==SETTING_MAX) cfg->device=dev_count-1;

	std::vector<CameraConfig> cfg_list = videoInputCamera::getCameraConfigs(cfg->device);
	if (cfg_list.size()==0) return false;
	sprintf(cfg->name,cfg_list[0].name);
	if (cfg->cam_format==FORMAT_UNKNOWN) cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cfg,cfg_list);

    GUID mediaSubtype = getMediaSubtype(cfg->cam_format);
	VI->setRequestedMediaSubType(mediaSubtype);
	VI->setIdealFramerate(this->cfg->device, cfg->cam_fps);

	comInit();
	bool bOk = VI->setupDevice(cfg->device, cfg->cam_width, cfg->cam_height);

	if (bOk == true) {	
		cfg->cam_width = VI->getWidth(cfg->device);
		cfg->cam_height = VI->getHeight(cfg->device);
	} else return false;

	applyCameraSettings();
	setupFrame();

	if (cfg->frame) cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->src_format];
	else cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->src_format];

	return true;
}

bool videoInputCamera::startCamera()
{ 
	running = true;
	return true;
}

unsigned char* videoInputCamera::getFrame()
{

	if (VI->isFrameNew(cfg->device)){

		//unsigned char *src = VI->getPixels(cfg->device,false,false);
		unsigned char *src = VI->getPixelPointer(cfg->device);
		if (src==NULL) {
			if (lost_frames>timeout) { // give up after 1 second
				disconnect = true;
				running = false;
			} lost_frames++;
			Sleep(1);
			return NULL;
		}

		unsigned char *dest = cam_buffer;
		if (!cfg->color) {

			if (cfg->frame) flip_crop_rgb2gray(cfg->cam_width,src,dest);
			else flip_rgb2gray(cfg->cam_width,cfg->cam_height,src,dest);

		} else {

			if (cfg->frame) flip_crop(cfg->cam_width,cfg->cam_height,src,dest,cfg->buf_format);
			else flip(cfg->cam_width,cfg->cam_height,src,dest,cfg->buf_format);
		}

		lost_frames = 0;
		return cam_buffer;
	} else  {
		if (lost_frames>timeout) { // give up after 5 (at init) or 2 (at runtime) seconds
			disconnect = true;
			running = false;
		} lost_frames++;
		Sleep(1);
		return NULL;
	}
}

bool videoInputCamera::stopCamera()
{
	if(!running) return false;
	running = false;
	return true;
}
bool videoInputCamera::stillRunning() {
	return running;
}

bool videoInputCamera::resetCamera()
{
	return (stopCamera() && startCamera());

}

bool videoInputCamera::closeCamera()
{
	if (!disconnect) {
		updateSettings();
		CameraTool::saveSettings();
		VI->stopDevice(cfg->device);
		comUnInit();
	}
	return true;
}

void videoInputCamera::control(int key) {
	return;
}

bool videoInputCamera::showSettingsDialog(bool lock) {
	if (running) VI->showSettingsWindow(cfg->device);
	return lock;
}

bool videoInputCamera::hasCameraSetting(int mode) {
	switch (mode) {
	case BRIGHTNESS:
	case GAIN:
	case EXPOSURE:
	case SHARPNESS:
	case FOCUS:
	case GAMMA:
	case WHITE:
	case BACKLIGHT:
	case COLOR_HUE:
		return true;
	default:
		return false;
	}
}

bool videoInputCamera::hasCameraSettingAuto(int mode) {
	return true;
}

bool videoInputCamera::setCameraSettingAuto(int mode, bool flag) {

	long value=0;
	long setting = FLAGS_MANUAL;
	if (flag) setting = FLAGS_AUTO;

	switch (mode) {
	case BRIGHTNESS: VI->setVideoSettingFilter(cfg->device, VI->propBrightness, value, setting, false); break;
	case GAIN: VI->setVideoSettingFilter(cfg->device, VI->propGain, value, setting, false); break;
	case EXPOSURE: VI->setVideoSettingFilter(cfg->device, VI->propExposure, value, setting, false); break;
	case SHARPNESS: VI->setVideoSettingFilter(cfg->device, VI->propSharpness, value, setting, false); break;
	case FOCUS: VI->setVideoSettingFilter(cfg->device, VI->propFocus, value, setting, false); break;
	case GAMMA: VI->setVideoSettingFilter(cfg->device, VI->propGamma, value, setting, false); break;
	case WHITE: VI->setVideoSettingFilter(cfg->device, VI->propWhiteBalance, value, setting, false); break;
	case BACKLIGHT: VI->setVideoSettingFilter(cfg->device, VI->propBacklightCompensation, value, setting, false); break;
	case COLOR_HUE: VI->setVideoSettingFilter(cfg->device, VI->propHue, value, setting, false); break;
	default: return false;
	}

	return true;
}

bool videoInputCamera::getCameraSettingAuto(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->getVideoSettingFilter(cfg->device, VI->propBrightness, min, max,  step, value, flags, default); break;
	case GAIN:  VI->getVideoSettingFilter(cfg->device, VI->propGain, min, max,  step, value, flags, default); break;
	case EXPOSURE: VI->getVideoSettingFilter(cfg->device, VI->propExposure, min, max,  step, value, flags, default); break;
	case SHARPNESS: VI->getVideoSettingFilter(cfg->device, VI->propSharpness, min, max,  step, value, flags, default); break;
	case FOCUS:  VI->getVideoSettingFilter(cfg->device, VI->propFocus, min, max,  step, value, flags, default); break;
	case GAMMA: VI->getVideoSettingFilter(cfg->device, VI->propGamma, min, max,  step, value, flags, default); break;
	case WHITE: VI->setVideoSettingFilter(cfg->device, VI->propWhiteBalance, value, flags, default); break;
	case BACKLIGHT: VI->setVideoSettingFilter(cfg->device, VI->propBacklightCompensation, value, flags, default); break;
	case COLOR_HUE: VI->setVideoSettingFilter(cfg->device, VI->propHue, value, flags, default); break;
	default: return false;
	} 

	if (flags==FLAGS_AUTO) return true;
	else return false;
}


bool videoInputCamera::setCameraSetting(int mode, int setting) {

	int current_setting = getCameraSetting(mode);
	if (setting==current_setting) return true;

	long flags = FLAGS_MANUAL;

	switch (mode) {
	case BRIGHTNESS: VI->setVideoSettingFilter(cfg->device, VI->propBrightness, setting, flags, false); break;
	case GAIN: VI->setVideoSettingFilter(cfg->device, VI->propGain, setting, flags, false); break;
	case EXPOSURE: VI->setVideoSettingFilter(cfg->device, VI->propExposure, setting, flags, false); break;
	case SHARPNESS: VI->setVideoSettingFilter(cfg->device, VI->propSharpness, setting, flags, false); break;
	case FOCUS: VI->setVideoSettingFilter(cfg->device, VI->propFocus, setting, flags, false); break;
	case GAMMA: VI->setVideoSettingFilter(cfg->device, VI->propGamma, setting, flags, false); break;
	case WHITE: VI->setVideoSettingFilter(cfg->device, VI->propWhiteBalance, setting, flags, false); break;
	case BACKLIGHT: VI->setVideoSettingFilter(cfg->device, VI->propBacklightCompensation, setting, flags, false); break;
	case COLOR_HUE: VI->setVideoSettingFilter(cfg->device, VI->propHue, setting, flags, false); break;
	}

	return true;
}

int videoInputCamera::getCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->getVideoSettingFilter(cfg->device, VI->propBrightness, min, max,  step, value, flags, default); break;
	case GAIN:  VI->getVideoSettingFilter(cfg->device, VI->propGain, min, max,  step, value, flags, default); break;
	case EXPOSURE: VI->getVideoSettingFilter(cfg->device, VI->propExposure, min, max,  step, value, flags, default); break;
	case SHARPNESS: VI->getVideoSettingFilter(cfg->device, VI->propSharpness, min, max,  step, value, flags, default); break;
	case FOCUS:  VI->getVideoSettingFilter(cfg->device, VI->propFocus, min, max,  step, value, flags, default); break;
	case GAMMA: VI->getVideoSettingFilter(cfg->device, VI->propGamma, min, max,  step, value, flags, default); break;
	case WHITE: VI->getVideoSettingFilter(cfg->device, VI->propWhiteBalance, min, max,  step, value, flags, default); break;
	case BACKLIGHT:  VI->getVideoSettingFilter(cfg->device, VI->propBacklightCompensation, min, max,  step, value, flags, default); break;
	case COLOR_HUE: VI->getVideoSettingFilter(cfg->device, VI->propHue, min, max,  step, value, flags, default); break;
	default: return 0;
	} 

	return (int)value;
}

int videoInputCamera::getMaxCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->getVideoSettingFilter(cfg->device, VI->propBrightness, min, max,  step, value, flags, default); break;
	case GAIN:  VI->getVideoSettingFilter(cfg->device, VI->propGain, min, max,  step, value, flags, default); break;
	case EXPOSURE: VI->getVideoSettingFilter(cfg->device, VI->propExposure, min, max,  step, value, flags, default); break;
	case SHARPNESS: VI->getVideoSettingFilter(cfg->device, VI->propSharpness, min, max,  step, value, flags, default); break;
	case FOCUS:  VI->getVideoSettingFilter(cfg->device, VI->propFocus, min, max,  step, value, flags, default); break;
	case GAMMA: VI->getVideoSettingFilter(cfg->device, VI->propGamma, min, max,  step, value, flags, default); break;
	case WHITE: VI->getVideoSettingFilter(cfg->device, VI->propWhiteBalance, min, max,  step, value, flags, default); break;
	case BACKLIGHT:  VI->getVideoSettingFilter(cfg->device, VI->propBacklightCompensation, min, max,  step, value, flags, default); break;
	case COLOR_HUE: VI->getVideoSettingFilter(cfg->device, VI->propHue, min, max,  step, value, flags, default); break;
	default: return 0;
	} 

	return (int)max;
}

int videoInputCamera::getMinCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->getVideoSettingFilter(cfg->device, VI->propBrightness, min, max,  step, value, flags, default); break;
	case GAIN:  VI->getVideoSettingFilter(cfg->device, VI->propGain, min, max,  step, value, flags, default); break;
	case EXPOSURE: VI->getVideoSettingFilter(cfg->device, VI->propExposure, min, max,  step, value, flags, default); break;
	case SHARPNESS: VI->getVideoSettingFilter(cfg->device, VI->propSharpness, min, max,  step, value, flags, default); break;
	case FOCUS:  VI->getVideoSettingFilter(cfg->device, VI->propFocus, min, max,  step, value, flags, default); break;
	case GAMMA: VI->getVideoSettingFilter(cfg->device, VI->propGamma, min, max,  step, value, flags, default); break;
	case WHITE: VI->getVideoSettingFilter(cfg->device, VI->propWhiteBalance, min, max,  step, value, flags, default); break;
	case BACKLIGHT:  VI->getVideoSettingFilter(cfg->device, VI->propBacklightCompensation, min, max,  step, value, flags, default); break;
	case COLOR_HUE: VI->getVideoSettingFilter(cfg->device, VI->propHue, min, max,  step, value, flags, default); break;
	default: return 0;
	} 

	return (int)min;
}

int videoInputCamera::getCameraSettingStep(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->getVideoSettingFilter(cfg->device, VI->propBrightness, min, max,  step, value, flags, default); break;
	case GAIN:  VI->getVideoSettingFilter(cfg->device, VI->propGain, min, max,  step, value, flags, default); break;
	case EXPOSURE: VI->getVideoSettingFilter(cfg->device, VI->propExposure, min, max,  step, value, flags, default); break;
	case SHARPNESS: VI->getVideoSettingFilter(cfg->device, VI->propSharpness, min, max,  step, value, flags, default); break;
	case FOCUS:  VI->getVideoSettingFilter(cfg->device, VI->propFocus, min, max,  step, value, flags, default); break;
	case GAMMA: VI->getVideoSettingFilter(cfg->device, VI->propGamma, min, max,  step, value, flags, default); break;
	case WHITE: VI->getVideoSettingFilter(cfg->device, VI->propWhiteBalance, min, max,  step, value, flags, default); break;
	case BACKLIGHT:  VI->getVideoSettingFilter(cfg->device, VI->propBacklightCompensation, min, max,  step, value, flags, default); break;
	case COLOR_HUE: VI->getVideoSettingFilter(cfg->device, VI->propHue, min, max,  step, value, flags, default); break;
	default: return 0;
	} 

	return (int)step;
}

bool videoInputCamera::setDefaultCameraSetting(int mode) {

	long value=0;
	long flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->setVideoSettingFilter(cfg->device,  VI->propBrightness, value, flags, true); break;
	case GAIN: VI->setVideoSettingFilter(cfg->device,  VI->propGain, value, flags, true); break;
	case EXPOSURE: VI->setVideoSettingFilter(cfg->device,  VI->propExposure, value, flags, true); break;
	case SHARPNESS:VI->setVideoSettingFilter(cfg->device,  VI->propSharpness, value, flags, true); break;
	case FOCUS: VI->setVideoSettingFilter(cfg->device,  VI->propFocus, value, flags, true); break;
	case GAMMA: VI->setVideoSettingFilter(cfg->device,  VI->propGamma, value, flags, true); break;
	case WHITE: VI->setVideoSettingFilter(cfg->device, VI->propWhiteBalance, value, flags, true); break;
	case BACKLIGHT: VI->setVideoSettingFilter(cfg->device, VI->propBacklightCompensation, value, flags, true); break;
	case COLOR_HUE: VI->setVideoSettingFilter(cfg->device, VI->propHue, value, flags, true); break;
	default: return false;
	}

	return true;
}

int videoInputCamera::getDefaultCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
	case BRIGHTNESS: VI->getVideoSettingFilter(cfg->device, VI->propBrightness, min, max,  step, value, flags, default); break;
	case GAIN:  VI->getVideoSettingFilter(cfg->device, VI->propGain, min, max,  step, value, flags, default); break;
	case EXPOSURE: VI->getVideoSettingFilter(cfg->device, VI->propExposure, min, max,  step, value, flags, default); break;
	case SHARPNESS: VI->getVideoSettingFilter(cfg->device, VI->propSharpness, min, max,  step, value, flags, default); break;
	case FOCUS:  VI->getVideoSettingFilter(cfg->device, VI->propFocus, min, max,  step, value, flags, default); break;
	case GAMMA: VI->getVideoSettingFilter(cfg->device, VI->propGamma, min, max,  step, value, flags, default); break;
	case WHITE: VI->getVideoSettingFilter(cfg->device, VI->propWhiteBalance, min, max,  step, value, flags, default); break;
	case BACKLIGHT:  VI->getVideoSettingFilter(cfg->device, VI->propBacklightCompensation, min, max,  step, value, flags, default); break;
	case COLOR_HUE: VI->getVideoSettingFilter(cfg->device, VI->propHue, min, max,  step, value, flags, default); break;
	default: return 0;
	} 

	return (int)default;
}