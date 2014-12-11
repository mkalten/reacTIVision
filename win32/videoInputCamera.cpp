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

#include "videoInputCamera.h"

videoInputCamera::videoInputCamera(const char *cfg) : CameraEngine (cfg)
{
	VI = new videoInput();
	VI->setVerbose(false);
	cam_buffer = NULL;
	running = false;

	timeout= 1000;
	lost_frames=0;
	disconnect = false;
}

videoInputCamera::~videoInputCamera()
{
	delete VI;
	if (cam_buffer!=NULL) delete cam_buffer;
}

void videoInputCamera::listDevices() {

	videoInput VI;
	std::vector <std::string> devList = VI.getDeviceList();

	int count = devList.size();
	if (count==0) { std::cout << " no videoInput camera found!" << std::endl; return; }
	else if (count==1) std::cout << "1 videoInput camera found:" << std::endl;
	else std::cout << count << " videoInput cameras found:" << std::endl;
	
	for(int i = 0; i <count; i++){
		std::cout << "\t" << i << ": " << devList[i] << std::endl;
	}
}

bool videoInputCamera::findCamera() {

	readSettings();
	std::vector <std::string> devList = VI->getDeviceList();

	int count = devList.size();
	if (count==0) { std::cout << " no videoInput camera found!" << std::endl; return false; }
	else if (count==1) std::cout << "1 videoInput camera found" << std::endl;
	else std::cout << count << " videoInput cameras found" << std::endl;
	
	if (config.device<0) config.device=0;
	if (config.device>=count) config.device = count-1;

	cameraID = config.device;
	sprintf(cameraName,devList[cameraID].c_str());
	return true;
}

bool videoInputCamera::initCamera() {

	// TODO: get actual video formats
	if (config.cam_width<=0) config.cam_width=640;
	if (config.cam_height<=0) config.cam_height=480;
	if (config.cam_fps<=0) config.cam_fps=30;

	VI->setIdealFramerate(this->cameraID, config.cam_fps);
	if (config.compress == true) VI->setRequestedMediaSubType(VI_MEDIASUBTYPE_MJPG);
	bool bOk = VI->setupDevice(cameraID, config.cam_width, config.cam_height);

	if (bOk == true) {	
		this->cam_width = VI->getWidth(cameraID);
		this->cam_height = VI->getHeight(cameraID);
		this->fps = config.cam_fps;
	} else return false;


	applyCameraSettings();
	setupFrame();
	if (config.frame) cam_buffer = new unsigned char[this->cam_width*this->cam_height];
	else cam_buffer = new unsigned char[this->cam_width*this->cam_height];
	return true;
}

bool videoInputCamera::startCamera()
{ 
	running = true;
	return true;
}

unsigned char* videoInputCamera::getFrame()
{

	if (VI->isFrameNew(cameraID)){

		//unsigned char *src = VI->getPixels(cameraID,false,false);
		unsigned char *src = VI->getPixelPointer(cameraID);
		if (src==NULL) {
			if (lost_frames>timeout) { // give up after 1 second
				disconnect = true;
				running = false;
			} lost_frames++;
			Sleep(1);
			return NULL;
		}

		unsigned char *dest = cam_buffer;
		unsigned char r,g,b;

		if (config.frame) {
			    src += 3*(config.frame_yoff*cam_width);
				dest += frame_width*frame_height-1;
                int xend = (cam_width-(frame_width+config.frame_xoff));

                for (int i=0;i<frame_height;i++) {

                    src +=  3*config.frame_xoff;
                    for (int j=frame_width;j>0;j--) {
 						r = *src++;
						g = *src++;
						b = *src++;
						*dest-- = hibyte(r * 77 + g * 151 + b * 28);
                    }
                    src +=  3*xend;
                }
		} else {
			int size = cam_width*cam_height;
			dest += size-1;
			for(int i=size;i>0;i--) {
				r = *src++;
				g = *src++;
				b = *src++;
				*dest-- = hibyte(r * 77 + g * 151 + b * 28);
			}
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
		saveSettings();
		VI->stopDevice(cameraID);
	}
	return true;
}

void videoInputCamera::control(int key) {
	 return;
}

void videoInputCamera::showSettingsDialog() {
	if (running) VI->showSettingsWindow(cameraID);
}

bool videoInputCamera::setCameraSettingAuto(int mode, bool flag) {
	return true;
}

bool videoInputCamera::getCameraSettingAuto(int mode) {
    
	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
		case BRIGHTNESS: VI->getVideoSettingFilter(cameraID, VI->propBrightness, min, max,  step, value, flags, default); break;
		case GAIN:  VI->getVideoSettingFilter(cameraID, VI->propGain, min, max,  step, value, flags, default); break;
		case EXPOSURE: VI->getVideoSettingFilter(cameraID, VI->propExposure, min, max,  step, value, flags, default); break;
		case SHARPNESS: VI->getVideoSettingFilter(cameraID, VI->propSharpness, min, max,  step, value, flags, default); break;
		case FOCUS:  VI->getVideoSettingFilter(cameraID, VI->propFocus, min, max,  step, value, flags, default); break;
		case GAMMA: VI->getVideoSettingFilter(cameraID, VI->propGamma, min, max,  step, value, flags, default); break;
		default: return 0;
	} 

	return (int)flags;
}


bool videoInputCamera::setCameraSetting(int mode, int setting) {

	int current_setting = getCameraSetting(mode);
	if (setting==current_setting) return true;
	setCameraSettingAuto(mode,false);

	long flags = 1;

	switch (mode) {
		case BRIGHTNESS: VI->setVideoSettingFilter(cameraID, VI->propBrightness, setting, NULL, false); break;
		case GAIN: VI->setVideoSettingFilter(cameraID, VI->propGain, setting, NULL, false); break;
		case EXPOSURE: VI->setVideoSettingFilter(cameraID, VI->propExposure, setting, flags, false); break;
		case SHARPNESS: VI->setVideoSettingFilter(cameraID, VI->propSharpness, setting, NULL, false); break;
		case FOCUS: VI->setVideoSettingFilter(cameraID, VI->propFocus, setting, NULL, false); break;
		case GAMMA: VI->setVideoSettingFilter(cameraID, VI->propGamma, setting, NULL, false); break;
		default: return false;
	}

	return true;
}

int videoInputCamera::getCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
		case BRIGHTNESS: VI->getVideoSettingFilter(cameraID, VI->propBrightness, min, max,  step, value, flags, default); break;
		case GAIN:  VI->getVideoSettingFilter(cameraID, VI->propGain, min, max,  step, value, flags, default); break;
		case EXPOSURE: VI->getVideoSettingFilter(cameraID, VI->propExposure, min, max,  step, value, flags, default); break;
		case SHARPNESS: VI->getVideoSettingFilter(cameraID, VI->propSharpness, min, max,  step, value, flags, default); break;
		case FOCUS:  VI->getVideoSettingFilter(cameraID, VI->propFocus, min, max,  step, value, flags, default); break;
		case GAMMA: VI->getVideoSettingFilter(cameraID, VI->propGamma, min, max,  step, value, flags, default); break;
		default: return 0;
	} 

	return (int)value;
}

int videoInputCamera::getMaxCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;

	switch (mode) {
		case BRIGHTNESS: VI->getVideoSettingFilter(cameraID, VI->propBrightness, min, max,  step, value, flags, default); break;
		case GAIN:  VI->getVideoSettingFilter(cameraID, VI->propGain, min, max,  step, value, flags, default); break;
		case EXPOSURE: VI->getVideoSettingFilter(cameraID, VI->propExposure, min, max,  step, value, flags, default); break;
		case SHARPNESS: VI->getVideoSettingFilter(cameraID, VI->propSharpness, min, max,  step, value, flags, default); break;
		case FOCUS:  VI->getVideoSettingFilter(cameraID, VI->propFocus, min, max,  step, value, flags, default); break;
		case GAMMA: VI->getVideoSettingFilter(cameraID, VI->propGamma, min, max,  step, value, flags, default); break;
		default: return 0;
	} 

	return (int)max;
}

int videoInputCamera::getMinCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;
	
	switch (mode) {
		case BRIGHTNESS: VI->getVideoSettingFilter(cameraID, VI->propBrightness, min, max,  step, value, flags, default); break;
		case GAIN:  VI->getVideoSettingFilter(cameraID, VI->propGain, min, max,  step, value, flags, default); break;
		case EXPOSURE: VI->getVideoSettingFilter(cameraID, VI->propExposure, min, max,  step, value, flags, default); break;
		case SHARPNESS: VI->getVideoSettingFilter(cameraID, VI->propSharpness, min, max,  step, value, flags, default); break;
		case FOCUS:  VI->getVideoSettingFilter(cameraID, VI->propFocus, min, max,  step, value, flags, default); break;
		case GAMMA: VI->getVideoSettingFilter(cameraID, VI->propGamma, min, max,  step, value, flags, default); break;
		default: return 0;
	} 

	return (int)min;
}

int videoInputCamera::getCameraSettingStep(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;
	
	switch (mode) {
		case BRIGHTNESS: VI->getVideoSettingFilter(cameraID, VI->propBrightness, min, max,  step, value, flags, default); break;
		case GAIN:  VI->getVideoSettingFilter(cameraID, VI->propGain, min, max,  step, value, flags, default); break;
		case EXPOSURE: VI->getVideoSettingFilter(cameraID, VI->propExposure, min, max,  step, value, flags, default); break;
		case SHARPNESS: VI->getVideoSettingFilter(cameraID, VI->propSharpness, min, max,  step, value, flags, default); break;
		case FOCUS:  VI->getVideoSettingFilter(cameraID, VI->propFocus, min, max,  step, value, flags, default); break;
		case GAMMA: VI->getVideoSettingFilter(cameraID, VI->propGamma, min, max,  step, value, flags, default); break;
		default: return 0;
	} 

	return (int)step;
}

bool videoInputCamera::setDefaultCameraSetting(int mode) {
    
	long value=0;

	switch (mode) {
		case BRIGHTNESS: VI->setVideoSettingFilter(cameraID,  VI->propBrightness, value, NULL, true); break;
		case GAIN: VI->setVideoSettingFilter(cameraID,  VI->propGain, value, NULL, true); break;
		case EXPOSURE: VI->setVideoSettingFilter(cameraID,  VI->propExposure, value, NULL, true); break;
		case SHARPNESS:VI->setVideoSettingFilter(cameraID,  VI->propSharpness, value, NULL, true); break;
		case FOCUS: VI->setVideoSettingFilter(cameraID,  VI->propFocus, value, NULL, true); break;
		case GAMMA: VI->setVideoSettingFilter(cameraID,  VI->propGamma, value, NULL, true); break;
		default: return false;
	}

    return true;
}

int videoInputCamera::getDefaultCameraSetting(int mode) {

	long min,max,step,value,default,flags;
	min=max=step=value=default=flags=0;
	
	switch (mode) {
		case BRIGHTNESS: VI->getVideoSettingFilter(cameraID, VI->propBrightness, min, max,  step, value, flags, default); break;
		case GAIN:  VI->getVideoSettingFilter(cameraID, VI->propGain, min, max,  step, value, flags, default); break;
		case EXPOSURE: VI->getVideoSettingFilter(cameraID, VI->propExposure, min, max,  step, value, flags, default); break;
		case SHARPNESS: VI->getVideoSettingFilter(cameraID, VI->propSharpness, min, max,  step, value, flags, default); break;
		case FOCUS:  VI->getVideoSettingFilter(cameraID, VI->propFocus, min, max,  step, value, flags, default); break;
		case GAMMA: VI->getVideoSettingFilter(cameraID, VI->propGamma, min, max,  step, value, flags, default); break;
		default: return 0;
	} 

	return (int)default;
}

void videoInputCamera::updateSettings()  {
 
    int brightness = getCameraSetting(BRIGHTNESS);
    if (brightness==getMinCameraSetting(BRIGHTNESS)) config.brightness=SETTING_MIN;
    if (brightness==getMaxCameraSetting(BRIGHTNESS)) config.brightness=SETTING_MAX;
    if (brightness==getDefaultCameraSetting(BRIGHTNESS)) config.brightness=SETTING_DEFAULT;
   //printf("brightness %d\n",brightness);
    
    int contrast = getCameraSetting(CONTRAST);
    if (contrast==getMinCameraSetting(CONTRAST)) config.contrast=SETTING_MIN;
    if (contrast==getMaxCameraSetting(CONTRAST)) config.contrast=SETTING_MAX;
    if (contrast==getDefaultCameraSetting(CONTRAST)) config.contrast=SETTING_DEFAULT;
   //printf("contrast %d\n",contrast);
    
    int gain = getCameraSetting(GAIN);
    if (gain==getMinCameraSetting(GAIN)) config.gain=SETTING_MIN;
    if (gain==getMaxCameraSetting(GAIN)) config.gain=SETTING_MAX;
    if (gain==getDefaultCameraSetting(GAIN)) config.gain=SETTING_DEFAULT;
    //printf("gain %d\n",gain);
    
    int exposure = getCameraSetting(EXPOSURE);
    if (exposure==getMinCameraSetting(EXPOSURE)) config.exposure=SETTING_MIN;
    if (exposure==getMaxCameraSetting(EXPOSURE)) config.exposure=SETTING_MAX;
    if (exposure==getDefaultCameraSetting(EXPOSURE)) config.exposure=SETTING_DEFAULT;
    if (getCameraSettingAuto(EXPOSURE)==true) config.exposure=SETTING_AUTO;
    //printf("exposure %d\n",exposure);
    
    int sharpness = getCameraSetting(SHARPNESS);
    if (sharpness==getMinCameraSetting(SHARPNESS)) config.sharpness=SETTING_MIN;
    if (sharpness==getMaxCameraSetting(SHARPNESS)) config.sharpness=SETTING_MAX;
    if (sharpness==getDefaultCameraSetting(SHARPNESS)) config.sharpness=SETTING_DEFAULT;
    //printf("sharpness %d\n",sharpness);

    int focus = getCameraSetting(FOCUS);
    if (focus==getMinCameraSetting(FOCUS)) config.focus=SETTING_MIN;
    if (focus==getMaxCameraSetting(FOCUS)) config.focus=SETTING_MAX;
    if (focus==getDefaultCameraSetting(FOCUS)) config.focus=SETTING_DEFAULT;
    //printf("focus %d\n",focus);
    
    int gamma = getCameraSetting(GAMMA);
    if (gamma==getMinCameraSetting(GAMMA)) config.gamma=SETTING_MIN;
    if (gamma==getMaxCameraSetting(GAMMA)) config.gamma=SETTING_MAX;
    if (getCameraSettingAuto(GAMMA)==true) config.gamma=SETTING_AUTO;
    if (gamma==getDefaultCameraSetting(GAMMA)) config.gamma=SETTING_DEFAULT;
    //printf("gamma %d\n",gamma);
}