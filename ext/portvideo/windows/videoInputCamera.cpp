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
#include "CameraTool.h"

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

	std::vector <std::string> devList = VI->getDeviceList();

	int count = devList.size();
	if (count==0) { std::cout << " no videoInput camera found!" << std::endl; return false; }
	else if (count==1) std::cout << "1 videoInput camera found" << std::endl;
	else std::cout << count << " videoInput cameras found" << std::endl;
	
	if (cfg->device<0) cfg->device=0;
	if (cfg->device>=count) cfg->device = count-1;

	sprintf(cameraName,devList[cfg->device].c_str());
	return true;
}

bool videoInputCamera::initCamera() {

	// TODO: get actual video formats
	if (cfg->cam_width<=0) cfg->cam_width=640;
	if (cfg->cam_height<=0) cfg->cam_height=480;
	if (cfg->cam_fps<=0) cfg->cam_fps=30;

	VI->setIdealFramerate(this->cfg->device, cfg->cam_fps);
	if (cfg->compress == true) VI->setRequestedMediaSubType(VI_MEDIASUBTYPE_MJPG);
	bool bOk = VI->setupDevice(cfg->device, cfg->cam_width, cfg->cam_height);

	if (bOk == true) {	
		cfg->cam_width = VI->getWidth(cfg->device);
		cfg->cam_height = VI->getHeight(cfg->device);
	} else return false;

	applyCameraSettings();
	setupFrame();
	
	if (cfg->frame) cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*bytes];
	else cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*bytes];

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

		if (cfg->frame) flip_crop(cfg->cam_width,cfg->cam_height,src,dest,bytes);
		else flip(cfg->cam_width,cfg->cam_height,src,dest,bytes);
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
		//CameraTool::saveSettings(cfg);
		VI->stopDevice(cfg->device);
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
		case WHITE: VI->setVideoSettingFilter(cfg->device, VI->propWhiteBalance, value, flags, false); break;
		case BACKLIGHT: VI->setVideoSettingFilter(cfg->device, VI->propBacklightCompensation, value, flags, false); break;
		case COLOR_HUE: VI->setVideoSettingFilter(cfg->device, VI->propHue, value, flags, false); break;
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