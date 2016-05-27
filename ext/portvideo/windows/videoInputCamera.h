/*  portVideo, a cross platform camera framework
	Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
	videoInput largely based on videoInput by Theo Watson <theo.watson@gmail.com>

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
#include <dshow.h>
#include <process.h>

#include "CameraEngine.h"
#include "CameraTool.h"

#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include <uuids.h>

// Due to a missing qedit.h in recent Platform SDKs, we've replicated the relevant contents here
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SampleCB(
		double SampleTime,
		IMediaSample *pSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE BufferCB(
		double SampleTime,
		BYTE *pBuffer,
		long BufferLen) = 0;

};

MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(
		BOOL OneShot) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetMediaType(
		const AM_MEDIA_TYPE *pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(
		AM_MEDIA_TYPE *pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(
		BOOL BufferThem) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(
		/* [out][in] */ long *pBufferSize,
		/* [out] */ long *pBuffer) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(
		/* [retval][out] */ IMediaSample **ppSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetCallback(
		ISampleGrabberCB *pCallback,
		long WhichMethodToCallback) = 0;

};

EXTERN_C const CLSID CLSID_SampleGrabber;
EXTERN_C const IID IID_ISampleGrabber;
EXTERN_C const CLSID CLSID_NullRenderer;

//////////////////////////////  CALLBACK  ////////////////////////////////
class SampleGrabberCallback : public ISampleGrabberCB{
public:

	SampleGrabberCallback(){
		InitializeCriticalSection(&critSection);

		bufferSetup 		= false;
		newFrame			= false;
		latestBufferLength 	= 0;

		hEvent = CreateEvent(NULL, true, false, NULL);
	}

	~SampleGrabberCallback(){
		ptrBuffer = NULL;
		DeleteCriticalSection(&critSection);
		CloseHandle(hEvent);
		if(bufferSetup){
			delete [] pixels;
		}
	}

	bool setupBuffer(int numBytesIn){
		if(bufferSetup){
			return false;
		}else{
			numBytes 			= numBytesIn;
			pixels 				= new unsigned char[numBytes];
			bufferSetup 		= true;
			newFrame			= false;
			latestBufferLength 	= 0;
		}
		return true;
	}

	STDMETHODIMP_(ULONG) AddRef() { return 1; }
	STDMETHODIMP_(ULONG) Release() { return 2; }

	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject){
		*ppvObject = static_cast<ISampleGrabberCB*>(this);
		return S_OK;
	}

	STDMETHODIMP SampleCB(double Time, IMediaSample *pSample){
		if(WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) return S_OK;

		HRESULT hr = pSample->GetPointer(&ptrBuffer);

		if(hr == S_OK){
			latestBufferLength = pSample->GetActualDataLength();
			if(latestBufferLength == numBytes){
				EnterCriticalSection(&critSection);
				memcpy(pixels, ptrBuffer, latestBufferLength);
				newFrame	= true;
				LeaveCriticalSection(&critSection);
				SetEvent(hEvent);
			}else{
				printf("ERROR: SampleCB() - buffer sizes do not match\n");
			}
		}

		return S_OK;
	}

	STDMETHODIMP BufferCB(double Time, BYTE *pBuffer, long BufferLen){
		return E_NOTIMPL;
	}

	int latestBufferLength;
	int numBytes;
	bool newFrame;
	bool bufferSetup;
	unsigned char * pixels;
	unsigned char * ptrBuffer;
	CRITICAL_SECTION critSection;
	HANDLE hEvent;
};
//////////////////////////////  CALLBACK  ////////////////////////////////

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

	static bool comInit();
	static bool comUnInit();
	static HRESULT getDevice(IBaseFilter **pSrcFilter, int deviceID, WCHAR * wDeviceName, char * nDeviceName);

	bool getVideoSettingValue(long prop, long &value, long &flag);
	bool setVideoSettingValue(long prop, long value, long flag);
	bool getVideoSettingRange(long prop, long &min, long &max, long &step, long &flag, long &dflt);

	bool getVideoControlValue(long prop, long &value, long &flag);
	bool setVideoControlValue(long prop, long value, long flag);
	bool getVideoControlRange(long prop, long &min, long &max, long &step, long &flag, long &dflt);

	static void __cdecl settingsThread(void * objPtr);
	static HRESULT ShowFilterPropertyPages(IBaseFilter *pFilter);

	GUID getMediaSubtype(int type);
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
	bool isFrameNew();

	ICaptureGraphBuilder2 *pCaptureGraphBuilder;
	IGraphBuilder *pGraphBuilder;
	IMediaControl *pMediaControl;
	IBaseFilter *pInputFilter;
	IBaseFilter *pGrabberFilter;
	IBaseFilter * pDestFilter;
	IAMStreamConfig *pStreamConfig;
	ISampleGrabber * pSampleGrabber;
	AM_MEDIA_TYPE * pAmMediaType;
	SampleGrabberCallback * sgCallback;
};

static int comCount = 0;

#endif
