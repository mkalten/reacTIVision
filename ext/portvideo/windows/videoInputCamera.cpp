/*  portVideo, a cross platform camera framework
	Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
	videoInputCamera based on videoInput by Theo Watson <theo.watson@gmail.com>

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

videoInputCamera::videoInputCamera(CameraConfig *cam_cfg):CameraEngine (cam_cfg)
{
	cam_buffer = NULL;
	running = false;

	pCaptureGraphBuilder = NULL;
	pGraphBuilder = NULL;
	pMediaControl = NULL;
	pInputFilter = NULL;
	pSampleGrabber = NULL;
	pDestFilter = NULL;
	pGrabberFilter = NULL;
	pStreamConfig = NULL;
	pAmMediaType = NULL;

	sgCallback = new SampleGrabberCallback();

	timeout= 2000;
	lost_frames=0;
	disconnect = true;
}

videoInputCamera::~videoInputCamera()
{
	if (cam_buffer!=NULL) delete cam_buffer;
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

std::vector<CameraConfig> videoInputCamera::getCameraConfigs(int dev_id) {

	std::vector<CameraConfig> cfg_list;

	int count = getDeviceCount();
	if (count==0) return cfg_list;

	comInit();

	HRESULT hr;
	ICaptureGraphBuilder2 *lpCaptureGraphBuilder;
	IGraphBuilder *lpGraphBuilder;
	IBaseFilter *lpInputFilter;
	IAMStreamConfig *lpStreamConfig;

	char 	nDeviceName[255];
	WCHAR 	wDeviceName[255];

	for (int cam_id=0;cam_id<count;cam_id++) {
		if ((dev_id>=0) && (dev_id!=cam_id)) continue;
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&lpCaptureGraphBuilder);
		if (FAILED(hr))	// FAILED is a macro that tests the return value
		{
			printf("ERROR - Could not create the Filter Graph Manager\n");
			comUnInit();
			return cfg_list;
		}

		// Create the Filter Graph Manager.
		hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,IID_IGraphBuilder, (void**)&lpGraphBuilder);
		if (FAILED(hr))
		{
			printf("ERROR - Could not add the graph builder!\n");
			lpCaptureGraphBuilder->Release();
			comUnInit();
			return cfg_list;
		}

		hr = lpCaptureGraphBuilder->SetFiltergraph(lpGraphBuilder);
		if (FAILED(hr))
		{
			printf("ERROR - Could not set filtergraph\n");
			lpGraphBuilder->Release();
			lpCaptureGraphBuilder->Release();
			comUnInit();
			return cfg_list;
		}

		memset(wDeviceName, 0, sizeof(WCHAR) * 255);
		memset(nDeviceName, 0, sizeof(char) * 255);
		hr = getDevice(&lpInputFilter, cam_id, wDeviceName, nDeviceName);

		if (SUCCEEDED(hr)){
			hr = lpGraphBuilder->AddFilter(lpInputFilter, wDeviceName);
		}else{
			printf("ERROR - Could not find specified video device\n");
			lpGraphBuilder->Release();
			lpCaptureGraphBuilder->Release();
			comUnInit();
			return cfg_list;
		}

		hr = lpCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, lpInputFilter, IID_IAMStreamConfig, (void **)&lpStreamConfig);
		if(FAILED(hr)){
			printf("ERROR: Couldn't config the stream!\n");
			lpInputFilter->Release();
			lpGraphBuilder->Release();
			lpCaptureGraphBuilder->Release();
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
		hr = lpStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);
		std::vector<CameraConfig> fmt_list;

		if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
		{
			GUID lastFormat = MEDIASUBTYPE_None;
			for (int iFormat = 0; iFormat < iCount; iFormat+=2)
			{
				VIDEO_STREAM_CONFIG_CAPS scc;
				AM_MEDIA_TYPE *pmtConfig;
				hr =  lpStreamConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
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

					if(stepX <= 1 || stepY <= 1) {

						cam_cfg.cam_width = scc.InputSize.cx;
						cam_cfg.cam_height = scc.InputSize.cy;

						int maxFrameInterval = scc.MaxFrameInterval;
						if (maxFrameInterval==0) maxFrameInterval = 10000000;
						float last_fps=-1;
						VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmtConfig->pbFormat;
						for (int iv=scc.MinFrameInterval;iv<=maxFrameInterval;iv=iv*2) {
							pVih->AvgTimePerFrame = iv;
							hr = lpStreamConfig->SetFormat(pmtConfig);
							if (hr==S_OK) { hr = lpStreamConfig->GetFormat(&pmtConfig);
							float fps = ((int)floor(100000000.0f/(float)pVih->AvgTimePerFrame + 0.5f))/10.0f;
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
								hr = lpStreamConfig->SetFormat(pmtConfig);
								if (hr==S_OK) { hr = lpStreamConfig->GetFormat(&pmtConfig);
								float fps = ((int)floor(100000000.0f/(float)pVih->AvgTimePerFrame + 0.5f))/10.0f;
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

		lpStreamConfig->Release();
		lpInputFilter->Release();
		lpGraphBuilder->Release();
		lpCaptureGraphBuilder->Release();
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
	if (cfg_list.size()==0) {
		if (cam_cfg->force) return new videoInputCamera(cam_cfg);
		else return NULL;
	}

	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);

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
	if (cfg_list.size()==0) {
			if (!cfg->force) return false;
	} else {
		if (cfg->cam_format==FORMAT_UNKNOWN) cfg->cam_format = cfg_list[0].cam_format;
		setMinMaxConfig(cfg,cfg_list);
	}

	HRESULT hr = setupDevice();
	if(FAILED(hr)) return false;

	setupFrame();

	if (cfg->frame) cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->src_format];
	else cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->src_format];

	return true;
}

HRESULT videoInputCamera::setupDevice() {

	comInit();
	GUID CAPTURE_MODE   = PIN_CATEGORY_CAPTURE; //Don't worry - it ends up being preview (which is faster)
	//printf("SETUP: Setting up device %i\n",deviceID);

	// CREATE THE GRAPH BUILDER //
	// Create the filter graph manager and query for interfaces.
	HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&pCaptureGraphBuilder);
	if (FAILED(hr))	// FAILED is a macro that tests the return value
	{
		printf("ERROR - Could not create the Filter Graph Manager\n");
		return hr;
	}

	//FITLER GRAPH MANAGER//
	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,IID_IGraphBuilder, (void**)&pGraphBuilder);
	if (FAILED(hr))
	{
		printf("ERROR - Could not add the graph builder!\n");
		stopDevice();
		return hr;
	}

	//SET THE FILTERGRAPH//
	hr = pCaptureGraphBuilder->SetFiltergraph(pGraphBuilder);
	if (FAILED(hr))
	{
		printf("ERROR - Could not set filtergraph\n");
		stopDevice();
		return hr;
	}

	//MEDIA CONTROL (START/STOPS STREAM)//
	// Using QueryInterface on the graph builder,
	// Get the Media Control object.
	hr = pGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	if (FAILED(hr))
	{
		printf("ERROR - Could not create the Media Control object\n");
		stopDevice();
		return hr;
	}

	char 	nDeviceName[255];
	WCHAR 	wDeviceName[255];
	memset(wDeviceName, 0, sizeof(WCHAR) * 255);
	memset(nDeviceName, 0, sizeof(char) * 255);

	//FIND VIDEO DEVICE AND ADD TO GRAPH//
	//gets the device specified by the second argument.
	hr = getDevice(&pInputFilter, cfg->device, wDeviceName, nDeviceName);

	if (SUCCEEDED(hr)){
		sprintf(cfg->name,nDeviceName);
		//printf("SETUP: %s\n", nDeviceName);
		hr = pGraphBuilder->AddFilter(pInputFilter, wDeviceName);
	}else{
		printf("ERROR - Could not find specified video device\n");
		stopDevice();
		return hr;
	}

	//LOOK FOR PREVIEW PIN IF THERE IS NONE THEN WE USE CAPTURE PIN AND THEN SMART TEE TO PREVIEW
	IAMStreamConfig *streamConfTest = NULL;
	hr = pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pInputFilter, IID_IAMStreamConfig, (void **)&streamConfTest);
	if(FAILED(hr)){
		//printf("SETUP: Couldn't find preview pin using SmartTee\n");
	}else{
		CAPTURE_MODE = PIN_CATEGORY_PREVIEW;
		streamConfTest->Release();
		streamConfTest = NULL;
	}

	//CROSSBAR (SELECT PHYSICAL INPUT TYPE)//
	//my own function that checks to see if the device can support a crossbar and if so it routes it.
	//webcams tend not to have a crossbar so this function will also detect a webcams and not apply the crossbar
	/*if(useCrossbar)
	{
	//printf("SETUP: Checking crossbar\n");
	routeCrossbar(pCaptureGraphBuilder, pInputFilter, connection, CAPTURE_MODE);
	}*/

	//we do this because webcams don't have a preview mode
	hr = pCaptureGraphBuilder->FindInterface(&CAPTURE_MODE, &MEDIATYPE_Video, pInputFilter, IID_IAMStreamConfig, (void **)&pStreamConfig);
	if(FAILED(hr)){
		printf("ERROR: Couldn't config the stream!\n");
		stopDevice();
		return hr;
	}

	//NOW LETS DEAL WITH GETTING THE RIGHT SIZE
	hr = pStreamConfig->GetFormat(&pAmMediaType);
	if(FAILED(hr)){
		printf("ERROR: Couldn't getFormat for pAmMediaType!\n");
		stopDevice();
		return hr;
	}

	if (!setSizeAndSubtype()) return false;

	VIDEOINFOHEADER *pVih =  reinterpret_cast<VIDEOINFOHEADER*>(pAmMediaType->pbFormat);
	cfg->cam_width	=  HEADER(pVih)->biWidth;
	cfg->cam_height	=  HEADER(pVih)->biHeight;
	cfg->cam_fps = ((int)floor(100000000.0f/(float)pVih->AvgTimePerFrame + 0.5f))/10.0f;

	long bufferSize = cfg->cam_width*cfg->cam_height*3;
	sgCallback->setupBuffer(bufferSize);

	// Create the Sample Grabber.
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,IID_IBaseFilter, (void**)&pGrabberFilter);
	if (FAILED(hr)){
		printf("Could not Create Sample Grabber - CoCreateInstance()\n");
		stopDevice();
		return hr;
	}

	hr = pGraphBuilder->AddFilter(pGrabberFilter, L"Sample Grabber");
	if (FAILED(hr)){
		printf("Could not add Sample Grabber - AddFilter()\n");
		stopDevice();
		return hr;
	}

	hr = pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&pSampleGrabber);
	if (FAILED(hr)){
		printf("ERROR: Could not query SampleGrabber\n");
		stopDevice();
		return hr;
	}

	//Get video properties from the stream's mediatype and apply to the grabber (otherwise we don't get an RGB image)
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt,sizeof(AM_MEDIA_TYPE));
	mt.majortype 	= MEDIATYPE_Video;
	mt.subtype 		= MEDIASUBTYPE_RGB24;
	//mt.subtype 		= MEDIASUBTYPE_YUY2;

	mt.formattype 	= FORMAT_VideoInfo;

	//Set Params - One Shot should be false unless you want to capture just one buffer
	hr = pSampleGrabber->SetMediaType(&mt);
	hr = pSampleGrabber->SetOneShot(FALSE);
	hr = pSampleGrabber->SetBufferSamples(FALSE);

	//Tell the grabber to use our callback function - 0 is for SampleCB and 1 for BufferCB
	//We use SampleCB
	hr = pSampleGrabber->SetCallback(sgCallback, 0);
	if (FAILED(hr)) {
		printf("ERROR: problem setting callback\n");
		stopDevice();
		return hr;
	} /*else {
		printf("SETUP: Capture callback set\n");
	  }*/


	//lets try freeing our stream conf here too
	//this will fail if the device is already running
	/* if(pStreamConfig) {
		pStreamConfig->Release();
		pStreamConfig = NULL;
	} else {
		printf("ERROR: connecting device - prehaps it is already being used?\n");
		stopDevice();
		return S_FALSE;
	}*/

	//used to give the video stream somewhere to go to.
	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&pDestFilter));
	if (FAILED(hr)){
		printf("ERROR: Could not create filter - NullRenderer\n");
		stopDevice();
		return hr;
	}

	hr = pGraphBuilder->AddFilter(pDestFilter, L"NullRenderer");
	if (FAILED(hr)){
		printf("ERROR: Could not add filter - NullRenderer\n");
		stopDevice();
		return hr;
	}

	//This is where the stream gets put together.
	hr = pCaptureGraphBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pInputFilter, pGrabberFilter, pDestFilter);

	if (FAILED(hr)){
		printf("ERROR: Could not connect pins - RenderStream()\n");
		stopDevice();
		return hr;
	}


	//EXP - lets try setting the sync source to null - and make it run as fast as possible
	{
		IMediaFilter *pMediaFilter = 0;
		hr = pGraphBuilder->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (FAILED(hr)){
			printf("ERROR: Could not get IID_IMediaFilter interface\n");
		}else{
			pMediaFilter->SetSyncSource(NULL);
			pMediaFilter->Release();
		}
	}

	//printf("SETUP: Device is setup and ready to capture.\n\n");

	//if we release this then we don't have access to the settings
	//we release our video input filter but then reconnect with it
	//each time we need to use it
	//pInputFilter->Release();
	//pInputFilter = NULL;

	pGrabberFilter->Release();
	pGrabberFilter = NULL;

	pDestFilter->Release();
	pDestFilter = NULL;

	return S_OK;
}

HRESULT videoInputCamera::stopDevice() {

	HRESULT HR = NULL;

	//Stop the callback and free it
	if( (sgCallback) && (pSampleGrabber) )
	{
		//printf("SETUP: freeing Grabber Callback\n");
		pSampleGrabber->SetCallback(NULL, 1);
		sgCallback->Release();
		delete sgCallback;
		sgCallback = NULL;
	}

	//Check to see if the graph is running, if so stop it.
	if( (pMediaControl) )
	{
		HR = pMediaControl->Pause();
		if (FAILED(HR)) printf("ERROR - Could not pause pControl\n");

		HR = pMediaControl->Stop();
		if (FAILED(HR)) printf("ERROR - Could not stop pControl\n");
	}

	//Disconnect filters from capture device
	if( (pInputFilter) ) NukeDownstream(pInputFilter);

	//Release and zero pointers to our filters etc
	if (pDestFilter) { 		//printf("SETUP: freeing Renderer \n");
		pDestFilter->Release();
		pDestFilter = NULL;
	}
	if (pInputFilter) { 		//printf("SETUP: freeing Capture Source \n");
		pInputFilter->Release();
		pInputFilter = NULL;
	}
	if (pGrabberFilter) {		//printf("SETUP: freeing Grabber Filter  \n");
		pGrabberFilter->Release();
		pGrabberFilter = NULL;
	}
	if (pSampleGrabber) {       //printf("SETUP: freeing Grabber  \n");
		pSampleGrabber->Release();
		pSampleGrabber = NULL;
	}
	if (pMediaControl) { 		//printf("SETUP: freeing Control   \n");
		pMediaControl->Release();
		pMediaControl = NULL;
	}
	if (pStreamConfig) { 		//printf("SETUP: freeing Stream  \n");
		pStreamConfig->Release();
		pStreamConfig = NULL;
	}

	if (pAmMediaType) { 		//printf("SETUP: freeing Media Type  \n");
		deleteMediaType(pAmMediaType);
	}

	//Destroy the graph
	if (pGraphBuilder) destroyGraph();

	//Release and zero our capture graph and our main graph
	if (pCaptureGraphBuilder) { 		//printf("SETUP: freeing Capture Graph \n");
		pCaptureGraphBuilder->Release();
		pCaptureGraphBuilder = NULL;
	}
	if (pGraphBuilder) { 			//printf("SETUP: freeing Main Graph \n");
		pGraphBuilder->Release();
		pGraphBuilder = NULL;
	}

	comUnInit();
	return S_OK;
}

bool videoInputCamera::startCamera()
{ 
	HRESULT hr = pMediaControl->Run();

	if (FAILED(hr)){
		//printf("ERROR: Could not start graph\n");
		stopDevice();
		return false;
	}

	applyCameraSettings();

	/*std::cout << std::flush;
	ResetEvent(sgCallback->hEvent);
	DWORD result = -1;
	int counter = 0;
	printf("init ");
	while ( result != WAIT_OBJECT_0) {
	result = WaitForSingleObject(sgCallback->hEvent, 100);

	counter++;
	printf(".");
	if (counter>100) return false;
	} printf(" done\n");
	ResetEvent(sgCallback->hEvent);*/

	disconnect = false;
	running = true;
	return true;
}

bool videoInputCamera::isFrameNew(){

	EnterCriticalSection(&sgCallback->critSection);
	if  (sgCallback->newFrame) {
		DWORD result = WaitForSingleObject(sgCallback->hEvent, 1000);
		if( result != WAIT_OBJECT_0) {
			LeaveCriticalSection(&sgCallback->critSection);
			return false;
		}

		ResetEvent(sgCallback->hEvent);
		sgCallback->newFrame = false;
		LeaveCriticalSection(&sgCallback->critSection);
		return true;
	}
	LeaveCriticalSection(&sgCallback->critSection);

	return false;
}
unsigned char* videoInputCamera::getFrame()
{

	if (isFrameNew()){

		unsigned char * src = sgCallback->pixels;
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
		//Sleep(1);
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
		stopDevice();
	}
	return true;
}

void videoInputCamera::control(int key) {
	return;
}

HRESULT videoInputCamera::ShowFilterPropertyPages(IBaseFilter *pFilter){

	ISpecifyPropertyPages *pProp;
	HRESULT hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if (SUCCEEDED(hr))
	{
		// Get the filter's name and IUnknown pointer.
		FILTER_INFO FilterInfo;
		hr = pFilter->QueryFilterInfo(&FilterInfo);
		IUnknown *pFilterUnk;
		pFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// Show the page.
		CAUUID caGUID;
		pProp->GetPages(&caGUID);
		pProp->Release();
		OleCreatePropertyFrame(
			NULL,                   // Parent window
			0, 0,                   // Reserved
			FilterInfo.achName,     // Caption for the dialog box
			1,                      // Number of objects (just the filter)
			&pFilterUnk,            // Array of object pointers.
			caGUID.cElems,          // Number of property pages
			caGUID.pElems,          // Array of property page CLSIDs
			0,                      // Locale identifier
			0, NULL                 // Reserved
			);

		// Clean up.
		if(pFilterUnk)pFilterUnk->Release();
		if(FilterInfo.pGraph)FilterInfo.pGraph->Release();
		CoTaskMemFree(caGUID.pElems);
	}
	return hr;
}

void __cdecl videoInputCamera::settingsThread(void * objPtr) {

	IBaseFilter *lpInputFilter = (IBaseFilter *)objPtr;
	ShowFilterPropertyPages(lpInputFilter);
	return;
}

bool videoInputCamera::showSettingsDialog(bool lock) {
	if (running) {
		HANDLE sThread = (HANDLE)_beginthread(settingsThread, 0, (void *)pInputFilter);
	}
	return lock;
}

bool videoInputCamera::hasCameraSetting(int mode) {

	long value,flags;
	value = flags = 0;

	switch (mode) {
	case BRIGHTNESS: return getVideoSettingValue(VideoProcAmp_Brightness, value, flags);
	case CONTRAST: return getVideoSettingValue(VideoProcAmp_Contrast, value, flags);
	case GAIN: return getVideoSettingValue(VideoProcAmp_Gain, value, flags);
	case EXPOSURE: return getVideoControlValue(CameraControl_Exposure, value, flags);
	case SHARPNESS: return getVideoSettingValue(VideoProcAmp_Sharpness, value, flags);
	case FOCUS: return getVideoControlValue(CameraControl_Focus, value, flags);
	case GAMMA: return getVideoSettingValue(VideoProcAmp_Gamma, value, flags);
	case WHITE: return getVideoSettingValue(VideoProcAmp_WhiteBalance, value, flags);
	case BACKLIGHT: return getVideoSettingValue(VideoProcAmp_BacklightCompensation, value, flags);
	case SATURATION: return getVideoSettingValue(VideoProcAmp_Saturation, value, flags);
	case COLOR_HUE: return getVideoSettingValue(VideoProcAmp_Hue, value, flags);
	case AUTO_GAIN: return hasCameraSettingAuto(GAIN);
	case AUTO_EXPOSURE: return hasCameraSettingAuto(EXPOSURE);
	case AUTO_FOCUS: return hasCameraSettingAuto(FOCUS);
	case AUTO_WHITE: return hasCameraSettingAuto(WHITE);
	case AUTO_HUE: return hasCameraSettingAuto(COLOR_HUE);
	default:
		return false;
	}
}

bool videoInputCamera::hasCameraSettingAuto(int mode) {

	long value,flags;
	value = flags = 0;

	switch (mode) {
	case GAIN: getVideoSettingValue(VideoProcAmp_Gain, value, flags); break;
	case EXPOSURE: getVideoControlValue(CameraControl_Exposure, value, flags); break;
	case FOCUS: getVideoControlValue(CameraControl_Focus, value, flags); break;
	case WHITE: getVideoSettingValue(VideoProcAmp_WhiteBalance, value, flags); break;
	case COLOR_HUE: getVideoSettingValue(VideoProcAmp_Hue, value, flags); break;
	default:
		return false;
	}

	if (flags==0) return false;
	else return true;
}

bool videoInputCamera::setCameraSettingAuto(int mode, bool flag) {

	long setting = VideoProcAmp_Flags_Manual;
	if (flag) setting = VideoProcAmp_Flags_Auto;

	switch (mode) {
	case BRIGHTNESS: setVideoSettingValue(VideoProcAmp_Brightness, getDefaultCameraSetting(BRIGHTNESS), setting); break;
	case CONTRAST: setVideoSettingValue(VideoProcAmp_Contrast, getDefaultCameraSetting(CONTRAST), setting); break;
	case GAIN: setVideoSettingValue(VideoProcAmp_Gain, getDefaultCameraSetting(GAIN), setting); break;
	case EXPOSURE: setVideoControlValue(CameraControl_Exposure, getDefaultCameraSetting(EXPOSURE), setting); break;
	case SHARPNESS: setVideoSettingValue(VideoProcAmp_Sharpness, getDefaultCameraSetting(SHARPNESS), setting); break;
	case FOCUS: setVideoControlValue(CameraControl_Focus, getDefaultCameraSetting(FOCUS), setting); break;
	case GAMMA: setVideoSettingValue(VideoProcAmp_Gamma, getDefaultCameraSetting(GAMMA), setting); break;
	case WHITE: setVideoSettingValue(VideoProcAmp_WhiteBalance, getDefaultCameraSetting(WHITE), setting); break;
	case BACKLIGHT: setVideoSettingValue(VideoProcAmp_BacklightCompensation, getDefaultCameraSetting(BACKLIGHT), setting); break;
	case SATURATION: setVideoSettingValue(VideoProcAmp_Saturation, getDefaultCameraSetting(SATURATION), setting); break;
	case COLOR_HUE: setVideoSettingValue(VideoProcAmp_Hue, getDefaultCameraSetting(COLOR_HUE), setting); break;
	default: return false;
	}

	return true;
}

bool videoInputCamera::getCameraSettingAuto(int mode) {

	long value,flags;
	value=flags=0;

	switch (mode) {
	case BRIGHTNESS: getVideoSettingValue(VideoProcAmp_Brightness, value, flags); break;
	case CONTRAST: getVideoSettingValue(VideoProcAmp_Contrast, value, flags); break;
	case GAIN:  getVideoSettingValue(VideoProcAmp_Gain, value, flags); break;
	case EXPOSURE: getVideoControlValue(CameraControl_Exposure, value, flags); break;
	case SHARPNESS: getVideoSettingValue(VideoProcAmp_Sharpness, value, flags); break;
	case FOCUS:  getVideoControlValue(CameraControl_Focus, value, flags); break;
	case GAMMA: getVideoSettingValue(VideoProcAmp_Gamma, value, flags); break;
	case WHITE: getVideoSettingValue(VideoProcAmp_WhiteBalance, value, flags); break;
	case BACKLIGHT: getVideoSettingValue(VideoProcAmp_BacklightCompensation, value, flags); break;
	case SATURATION: getVideoSettingValue(VideoProcAmp_Saturation, value, flags); break;
	case COLOR_HUE: getVideoSettingValue(VideoProcAmp_Hue, value, flags); break;
	default: return false;
	} 

	if (flags==VideoProcAmp_Flags_Auto) return true;
	else return false;
}


bool videoInputCamera::setCameraSetting(int mode, int setting) {

	int current_setting = getCameraSetting(mode);
	if (setting==current_setting) return true;

	long flags = VideoProcAmp_Flags_Manual;

	switch (mode) {
	case BRIGHTNESS: setVideoSettingValue(VideoProcAmp_Brightness, setting, flags); break;
	case CONTRAST: setVideoSettingValue(VideoProcAmp_Contrast, setting, flags); break;
	case GAIN: setVideoSettingValue(VideoProcAmp_Gain, setting, flags); break;
	case EXPOSURE: setVideoControlValue(CameraControl_Exposure, setting, flags); break;
	case SHARPNESS: setVideoSettingValue(VideoProcAmp_Sharpness, setting, flags); break;
	case FOCUS: setVideoControlValue(CameraControl_Focus, setting, flags); break;
	case GAMMA: setVideoSettingValue(VideoProcAmp_Gamma, setting, flags); break;
	case WHITE: setVideoSettingValue(VideoProcAmp_WhiteBalance, setting, flags); break;
	case BACKLIGHT: setVideoSettingValue(VideoProcAmp_BacklightCompensation, setting, flags); break;
	case SATURATION: setVideoSettingValue(VideoProcAmp_Saturation, setting, flags); break;
	case COLOR_HUE: setVideoSettingValue(VideoProcAmp_Hue, setting, flags); break;
	default: return false;
	}

	return true;
}

int videoInputCamera::getCameraSetting(int mode) {

	long value,flags;
	value=flags=0;

	switch (mode) {
	case BRIGHTNESS: getVideoSettingValue(VideoProcAmp_Brightness, value, flags); break;
	case CONTRAST: getVideoSettingValue(VideoProcAmp_Contrast, value, flags); break;
	case GAIN:  getVideoSettingValue(VideoProcAmp_Gain, value, flags); break;
	case EXPOSURE: getVideoControlValue(CameraControl_Exposure, value, flags); break;
	case SHARPNESS: getVideoSettingValue(VideoProcAmp_Sharpness, value, flags); break;
	case FOCUS:  getVideoControlValue(CameraControl_Focus, value, flags); break;
	case GAMMA: getVideoSettingValue(VideoProcAmp_Gamma, value, flags); break;
	case WHITE: getVideoSettingValue(VideoProcAmp_WhiteBalance, value, flags); break;
	case BACKLIGHT: getVideoSettingValue(VideoProcAmp_BacklightCompensation, value, flags); break;
	case SATURATION: getVideoSettingValue(VideoProcAmp_Saturation, value, flags); break;
	case COLOR_HUE: getVideoSettingValue(VideoProcAmp_Hue, value, flags); break;
	default: return 0;
	} 

	return (int)value;
}

int videoInputCamera::getMaxCameraSetting(int mode) {

	long min,max,step,dflt,flag;
	min=max=step=dflt=flag=0;

	switch (mode) {
	case BRIGHTNESS: getVideoSettingRange(VideoProcAmp_Brightness, min, max, step, flag, dflt); break;
	case CONTRAST: getVideoSettingRange(VideoProcAmp_Contrast, min, max, step, flag, dflt); break;
	case GAIN:  getVideoSettingRange(VideoProcAmp_Gain, min, max, step, flag, dflt); break;
	case EXPOSURE: getVideoControlRange(CameraControl_Exposure, min, max, step, flag, dflt); break;
	case SHARPNESS: getVideoSettingRange(VideoProcAmp_Sharpness, min, max, step, flag, dflt); break;
	case FOCUS:  getVideoControlRange(CameraControl_Focus, min, max, step, flag, dflt); break;
	case GAMMA: getVideoSettingRange(VideoProcAmp_Gamma, min, max, step, flag, dflt); break;
	case WHITE: getVideoSettingRange(VideoProcAmp_WhiteBalance, min, max, step, flag, dflt); break;
	case BACKLIGHT:  getVideoSettingRange(VideoProcAmp_BacklightCompensation, min, max, step, flag, dflt); break;
	case SATURATION: getVideoSettingRange(VideoProcAmp_Saturation, min, max, step, flag, dflt); break;
	case COLOR_HUE: getVideoSettingRange(VideoProcAmp_Hue, min, max, step, flag, dflt); break;
	default: return 0;
	} 

	return (int)max;
}

int videoInputCamera::getMinCameraSetting(int mode) {

	long min,max,step,dflt,flag;
	min=max=step=dflt=flag=0;

	switch (mode) {
	case BRIGHTNESS: getVideoSettingRange(VideoProcAmp_Brightness, min, max, step, flag, dflt); break;
	case CONTRAST: getVideoSettingRange(VideoProcAmp_Contrast, min, max, step, flag, dflt); break;
	case GAIN:  getVideoSettingRange(VideoProcAmp_Gain, min, max, step, flag, dflt); break;
	case EXPOSURE: getVideoControlRange(CameraControl_Exposure, min, max, step, flag, dflt); break;
	case SHARPNESS: getVideoSettingRange(VideoProcAmp_Sharpness, min, max, step, flag, dflt); break;
	case FOCUS:  getVideoControlRange(CameraControl_Focus, min, max, step, flag, dflt); break;
	case GAMMA: getVideoSettingRange(VideoProcAmp_Gamma, min, max, step, flag, dflt); break;
	case WHITE: getVideoSettingRange(VideoProcAmp_WhiteBalance, min, max, step, flag, dflt); break;
	case BACKLIGHT:  getVideoSettingRange(VideoProcAmp_BacklightCompensation, min, max, step, flag, dflt); break;
	case SATURATION: getVideoSettingRange(VideoProcAmp_Saturation, min, max, step, flag, dflt); break;
	case COLOR_HUE: getVideoSettingRange(VideoProcAmp_Hue, min, max, step, flag, dflt); break;
	default: return 0;
	} 

	return (int)min;
}

int videoInputCamera::getCameraSettingStep(int mode) {

	long min,max,step,dflt,flag;
	min=max=step=dflt=flag=0;

	switch (mode) {
	case BRIGHTNESS: getVideoSettingRange(VideoProcAmp_Brightness, min, max, step, flag, dflt); break;
	case CONTRAST: getVideoSettingRange(VideoProcAmp_Contrast, min, max, step, flag, dflt); break;
	case GAIN:  getVideoSettingRange(VideoProcAmp_Gain, min, max, step, flag, dflt); break;
	case EXPOSURE: getVideoControlRange(CameraControl_Exposure, min, max, step, flag, dflt); break;
	case SHARPNESS: getVideoSettingRange(VideoProcAmp_Sharpness, min, max, step, flag, dflt); break;
	case FOCUS:  getVideoControlRange(CameraControl_Focus, min, max, step, flag, dflt); break;
	case GAMMA: getVideoSettingRange(VideoProcAmp_Gamma, min, max, step, flag, dflt); break;
	case WHITE: getVideoSettingRange(VideoProcAmp_WhiteBalance, min, max, step, flag, dflt); break;
	case BACKLIGHT:  getVideoSettingRange(VideoProcAmp_BacklightCompensation, min, max, step, flag, dflt); break;
	case SATURATION: getVideoSettingRange(VideoProcAmp_Saturation, min, max, step, flag, dflt); break;
	case COLOR_HUE: getVideoSettingRange(VideoProcAmp_Hue, min, max, step, flag, dflt); break;
	default: return 0;
	} 

	return (int)step;
}

bool videoInputCamera::setDefaultCameraSetting(int mode) {

	switch (mode) {
	case BRIGHTNESS: setVideoSettingValue(VideoProcAmp_Brightness, getDefaultCameraSetting(BRIGHTNESS), VideoProcAmp_Flags_Manual); break;
	case CONTRAST: setVideoSettingValue(VideoProcAmp_Contrast, getDefaultCameraSetting(CONTRAST), VideoProcAmp_Flags_Manual); break;
	case GAIN: setVideoSettingValue(VideoProcAmp_Gain, getDefaultCameraSetting(GAIN), VideoProcAmp_Flags_Manual); break;
	case EXPOSURE: setVideoControlValue(CameraControl_Exposure, getDefaultCameraSetting(EXPOSURE), CameraControl_Flags_Manual); break;
	case SHARPNESS:setVideoSettingValue(VideoProcAmp_Sharpness, getDefaultCameraSetting(SHARPNESS), VideoProcAmp_Flags_Manual); break;
	case FOCUS: setVideoControlValue(CameraControl_Focus, getDefaultCameraSetting(FOCUS), CameraControl_Flags_Manual); break;
	case GAMMA: setVideoSettingValue(VideoProcAmp_Gamma, getDefaultCameraSetting(GAMMA), VideoProcAmp_Flags_Manual); break;
	case WHITE: setVideoSettingValue(VideoProcAmp_WhiteBalance, getDefaultCameraSetting(WHITE), VideoProcAmp_Flags_Manual); break;
	case BACKLIGHT: setVideoSettingValue(VideoProcAmp_BacklightCompensation, getDefaultCameraSetting(BACKLIGHT), VideoProcAmp_Flags_Manual); break;
	case SATURATION: setVideoSettingValue(VideoProcAmp_Saturation, getDefaultCameraSetting(SATURATION), VideoProcAmp_Flags_Manual); break;
	case COLOR_HUE: setVideoSettingValue(VideoProcAmp_Hue, getDefaultCameraSetting(COLOR_HUE), VideoProcAmp_Flags_Manual); break;
	default: return false;
	}

	return true;
}

int videoInputCamera::getDefaultCameraSetting(int mode) {

	long min,max,step,dflt,flag;
	min=max=step=dflt=flag=0;

	switch (mode) {
	case BRIGHTNESS: getVideoSettingRange(VideoProcAmp_Brightness, min, max, step, flag, dflt); break;
	case CONTRAST: getVideoSettingRange(VideoProcAmp_Contrast, min, max, step, flag, dflt); break;
	case GAIN:  getVideoSettingRange(VideoProcAmp_Gain, min, max, step, flag, dflt); break;
	case EXPOSURE: getVideoControlRange(CameraControl_Exposure, min, max, step, flag, dflt); break;
	case SHARPNESS: getVideoSettingRange(VideoProcAmp_Sharpness, min, max, step, flag, dflt); break;
	case FOCUS:  getVideoControlRange(CameraControl_Focus, min, max, step, flag, dflt); break;
	case GAMMA: getVideoSettingRange(VideoProcAmp_Gamma, min, max, step, flag, dflt); break;
	case WHITE: getVideoSettingRange(VideoProcAmp_WhiteBalance, min, max, step, flag, dflt); break;
	case BACKLIGHT:  getVideoSettingRange(VideoProcAmp_BacklightCompensation, min, max, step, flag, dflt); break;
	case SATURATION: getVideoSettingRange(VideoProcAmp_Saturation, min, max, step, flag, dflt); break;
	case COLOR_HUE: getVideoSettingRange(VideoProcAmp_Hue, min, max, step, flag, dflt); break;
	default: return 0;
	} 

	return (int)dflt;
}

bool videoInputCamera::getVideoSettingValue(long prop, long &value, long &flag){

	IAMVideoProcAmp *lpAMVideoProcAmp = NULL;
	HRESULT hr = pInputFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&lpAMVideoProcAmp);
	if (FAILED(hr)) return false;

	hr = lpAMVideoProcAmp->Get(prop, &value, &flag);
	lpAMVideoProcAmp->Release();

	if(FAILED(hr)) return false;
	else return true;
}

bool videoInputCamera::getVideoControlValue( long prop, long &value, long &flag){

	IAMCameraControl *lpCameraControl = NULL;
	HRESULT hr = pInputFilter->QueryInterface(IID_IAMCameraControl, (void**)&lpCameraControl);
	if (FAILED(hr)) return false;

	hr = lpCameraControl->Get(prop, &value, &flag);
	lpCameraControl->Release();

	if (FAILED(hr)) return false;
	else return true;
}

bool videoInputCamera::getVideoSettingRange(long prop, long &min, long &max, long &step, long &flag, long &dflt) {

	IAMVideoProcAmp *lpAMVideoProcAmp = NULL;
	HRESULT hr = pInputFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&lpAMVideoProcAmp);
	if (FAILED(hr)) return false;

	hr = lpAMVideoProcAmp->GetRange(prop, &min, &max, &step, &dflt, &flag);
	lpAMVideoProcAmp->Release();

	if (FAILED(hr)) return false;
	else return true;
}

bool videoInputCamera::getVideoControlRange(long prop, long &min, long &max, long &step, long &flag, long &dflt) {

	IAMCameraControl *lpCameraControl = NULL;
	HRESULT hr = pInputFilter->QueryInterface(IID_IAMCameraControl, (void**)&lpCameraControl);
	if (FAILED(hr)) return false;

	hr = lpCameraControl->GetRange(prop, &min, &max, &step, &dflt, &flag);
	lpCameraControl->Release();

	if (FAILED(hr)) return false;
	else return true;
}

bool videoInputCamera::setVideoSettingValue(long prop, long value, long flag){

	IAMVideoProcAmp *lpAMVideoProcAmp = NULL;
	HRESULT hr = pInputFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&lpAMVideoProcAmp);
	if (FAILED(hr)) return false;

	hr = lpAMVideoProcAmp->Set(prop, value, flag);
	lpAMVideoProcAmp->Release();

	if (FAILED(hr)) return false;
	else return true;
}

bool videoInputCamera::setVideoControlValue(long prop, long value, long flag){

	IAMCameraControl *lpCameraControl = NULL;
	HRESULT hr = pInputFilter->QueryInterface(IID_IAMCameraControl, (void**)&lpCameraControl);
	if (FAILED(hr)) return false;

	hr = lpCameraControl->Set(prop, value, flag);
	lpCameraControl->Release();

	if (FAILED(hr)) return false;
	else return true;
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
			pmt->pUnk->Release();
			pmt->pUnk = NULL;
		}

		CoTaskMemFree(pmt);
	}
}

bool videoInputCamera::comInit(){

	if(comCount == 0  ){
		HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
		if (FAILED(hr)) return false;
	}

	comCount++;
	return true;
}

bool videoInputCamera::comUnInit(){
	if(comCount > 0)comCount--;	

	if(comCount == 0){
		CoUninitialize();
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

void videoInputCamera::makeGUID( GUID *guid, unsigned long Data1, unsigned short Data2, unsigned short Data3,
								unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3,
								unsigned char b4, unsigned char b5, unsigned char b6, unsigned char b7 ) {

									guid->Data1 = Data1;
									guid->Data2 = Data2;
									guid->Data3 = Data3;
									guid->Data4[0] = b0; guid->Data4[1] = b1; guid->Data4[2] = b2; guid->Data4[3] = b3;
									guid->Data4[4] = b4; guid->Data4[5] = b5; guid->Data4[6] = b6; guid->Data4[7] = b7;
}


GUID videoInputCamera::getMediaSubtype(int type) {

	int iCount = 0;
	int iSize = 0;
	HRESULT hr = pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);

	if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
	{
		GUID lastFormat = MEDIASUBTYPE_None;
		for (int iFormat = 0; iFormat < iCount; iFormat+=2)
		{
			VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE *pmtConfig;
			hr =  pStreamConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
			if (SUCCEEDED(hr)){

				if ( pmtConfig->subtype != lastFormat) {

					if (type == getMediaSubtype(pmtConfig->subtype)) {

						GUID mediaSubtype = pmtConfig->subtype;
						deleteMediaType(pmtConfig);
						return mediaSubtype;
					}
					lastFormat = pmtConfig->subtype;
				}
				deleteMediaType(pmtConfig);
			}
		}
	}


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

void videoInputCamera::NukeDownstream(IBaseFilter *pBF){
	IPin *pP, *pTo;
	ULONG u;
	IEnumPins *pins = NULL;
	PIN_INFO pininfo;
	HRESULT hr = pBF->EnumPins(&pins);
	pins->Reset();
	while (hr == NOERROR)
	{
		hr = pins->Next(1, &pP, &u);
		if (hr == S_OK && pP)
		{
			pP->ConnectedTo(&pTo);
			if (pTo)
			{
				hr = pTo->QueryPinInfo(&pininfo);
				if (hr == NOERROR)
				{
					if (pininfo.dir == PINDIR_INPUT)
					{
						NukeDownstream(pininfo.pFilter);
						pGraphBuilder->Disconnect(pTo);
						pGraphBuilder->Disconnect(pP);
						pGraphBuilder->RemoveFilter(pininfo.pFilter);
					}
					pininfo.pFilter->Release();
					pininfo.pFilter = NULL;
				}
				pTo->Release();
			}
			pP->Release();
		}
	}
	if (pins) pins->Release();
}

void videoInputCamera::destroyGraph(){
	HRESULT hr = NULL;
	int FuncRetval=0;
	int NumFilters=0;

	int i = 0;
	while (hr == NOERROR)
	{
		IEnumFilters * pEnum = 0;
		ULONG cFetched;

		// We must get the enumerator again every time because removing a filter from the graph
		// invalidates the enumerator. We always get only the first filter from each enumerator.
		hr = pGraphBuilder->EnumFilters(&pEnum);
		//if (FAILED(hr)) { printf("SETUP: pGraph->EnumFilters() failed. \n"); return; }

		IBaseFilter * pFilter = NULL;
		if (pEnum->Next(1, &pFilter, &cFetched) == S_OK)
		{
			FILTER_INFO FilterInfo={0};
			hr = pFilter->QueryFilterInfo(&FilterInfo);
			FilterInfo.pGraph->Release();

			int count = 0;
			char buffer[255];
			memset(buffer, 0, 255 * sizeof(char));

			while( FilterInfo.achName[count] != 0x00 )
			{
				buffer[count] = static_cast<char>(FilterInfo.achName[count]);
				count++;
			}

			//printf("SETUP: removing filter %s...\n", buffer);
			hr = pGraphBuilder->RemoveFilter(pFilter);
			//if (FAILED(hr)) { printf("SETUP: pGraph->RemoveFilter() failed. \n"); return; }
			//printf("SETUP: filter removed %s  \n",buffer);

			pFilter->Release();
			pFilter = NULL;
		}
		else hr = 1;
		pEnum->Release();
		pEnum = NULL;
		i++;
	}

	return;
}

bool videoInputCamera::setSizeAndSubtype() {

	//store current mediatype
	AM_MEDIA_TYPE * tmpType = NULL;
	HRESULT	hr = pStreamConfig->GetFormat(&tmpType);
	if(hr != S_OK)return false;

	VIDEOINFOHEADER *lpVih =  reinterpret_cast<VIDEOINFOHEADER*>(pAmMediaType->pbFormat);
	HEADER(lpVih)->biWidth  = cfg->cam_width;
	HEADER(lpVih)->biHeight = cfg->cam_height;

	pAmMediaType->formattype = FORMAT_VideoInfo;
	pAmMediaType->majortype  = MEDIATYPE_Video;
	pAmMediaType->subtype	 = getMediaSubtype(cfg->cam_format);

	//buffer size
	pAmMediaType->lSampleSize = cfg->cam_width* cfg->cam_height*3;

	//set fps if requested
	lpVih->AvgTimePerFrame = (unsigned long)(10000000 /  cfg->cam_fps);

	//okay lets try new size
	hr = pStreamConfig->SetFormat(pAmMediaType);
	if(hr == S_OK){
		if( tmpType != NULL )deleteMediaType(tmpType);
		return true;
	}else{
		pStreamConfig->SetFormat(tmpType);
		if( tmpType != NULL )deleteMediaType(tmpType);
	}

	return false;
}

/*
HRESULT videoInputCamera::routeCrossbar(ICaptureGraphBuilder2 **ppBuild, IBaseFilter **pVidInFilter, int conType, GUID captureMode){

//create local ICaptureGraphBuilder2
ICaptureGraphBuilder2 *pBuild = NULL;
pBuild = *ppBuild;

//create local IBaseFilter
IBaseFilter *pVidFilter = NULL;
pVidFilter = * pVidInFilter;

// Search upstream for a crossbar.
IAMCrossbar *pXBar1 = NULL;
HRESULT hr = pBuild->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidFilter,
IID_IAMCrossbar, (void**)&pXBar1);
if (SUCCEEDED(hr))
{

bool foundDevice = false;

printf("SETUP: You are not a webcam! Setting Crossbar\n");
pXBar1->Release();

IAMCrossbar *Crossbar;
hr = pBuild->FindInterface(&captureMode, &MEDIATYPE_Interleaved, pVidFilter, IID_IAMCrossbar, (void **)&Crossbar);

if(hr != NOERROR){
hr = pBuild->FindInterface(&captureMode, &MEDIATYPE_Video, pVidFilter, IID_IAMCrossbar, (void **)&Crossbar);
}

LONG lInpin, lOutpin;
hr = Crossbar->get_PinCounts(&lOutpin , &lInpin);

BOOL IPin=TRUE; LONG pIndex=0 , pRIndex=0 , pType=0;

while( pIndex < lInpin)
{
hr = Crossbar->get_CrossbarPinInfo( IPin , pIndex , &pRIndex , &pType);

if( pType == conType){
printf("SETUP: Found Physical Interface");

switch(conType){

case PhysConn_Video_Composite:
printf(" - Composite\n");
break;
case PhysConn_Video_SVideo:
printf(" - S-Video\n");
break;
case PhysConn_Video_Tuner:
printf(" - Tuner\n");
break;
case PhysConn_Video_USB:
printf(" - USB\n");
break;
case PhysConn_Video_1394:
printf(" - Firewire\n");
break;
}

foundDevice = true;
break;
}
pIndex++;

}

if(foundDevice){
BOOL OPin=FALSE; LONG pOIndex=0 , pORIndex=0 , pOType=0;
while( pOIndex < lOutpin)
{
hr = Crossbar->get_CrossbarPinInfo( OPin , pOIndex , &pORIndex , &pOType);
if( pOType == PhysConn_Video_VideoDecoder)
break;
}
Crossbar->Route(pOIndex,pIndex);
}else{
printf("SETUP: Didn't find specified Physical Connection type. Using Defualt. \n");
}

//we only free the crossbar when we close or restart the device
//we were getting a crash otherwise
//if(Crossbar)Crossbar->Release();
//if(Crossbar)Crossbar = NULL;

if(pXBar1)pXBar1->Release();
if(pXBar1)pXBar1 = NULL;

}else{
printf("SETUP: You are a webcam or snazzy firewire cam! No Crossbar needed\n");
return hr;
}

return hr;
}
*/