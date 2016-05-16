/*  portVideo, a cross platform camera framework
    Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

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

	std::vector <std::string> devList = videoInput::getDeviceList();

	int count = devList.size();
	if (count==0) { std::cout << " no videoInput camera found!" << std::endl; return; }
	else if (count==1) std::cout << "1 videoInput camera found:" << std::endl;
	else std::cout << count << " videoInput cameras found:" << std::endl;
	
	/*for(int i = 0; i <count; i++){
		std::cout << "\t" << i << ": " << devList[i] << std::endl;
	}*/

	videoInput *tVI = new videoInput();
	videoInput::listDevicesAndFormats();
	delete tVI;
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

	if ((config.cam_width<0) || (config.cam_height<0)) {
		int min_width,max_width,min_height,max_height;
		min_width=max_width=min_height=max_height=0;
		videoInput::getMinMaxDimension(this->cameraID,min_width,max_width,min_height,max_height,config.compress);
		//std::cout << min_width << " " << min_height << " " << max_width << " " << max_height << " " << std::endl;

		if (config.cam_width==SETTING_MIN) config.cam_width=min_width;
		else if (config.cam_width==SETTING_MAX) config.cam_width=max_width;
		if (config.cam_height==SETTING_MIN) config.cam_height=min_height;
		else if (config.cam_height==SETTING_MAX) config.cam_height=max_height;
	}

	if (config.cam_fps<0) {
		float min_fps,max_fps;
		min_fps=max_fps=0.0f;
		videoInput::getMinMaxFramerate(this->cameraID,config.cam_width,config.cam_height,min_fps,max_fps,config.compress);
		//std::cout << config.cam_width << " " << config.cam_height << " " << min_fps << " " << max_fps << " " << std::endl;

		if (config.cam_fps==SETTING_MIN) config.cam_fps=min_fps;
		else if (config.cam_fps==SETTING_MAX) config.cam_fps=max_fps;
	}

	if (config.compress == true) VI->setRequestedMediaSubType(VI_MEDIASUBTYPE_MJPG);
	VI->setIdealFramerate(this->cameraID, config.cam_fps);
	bool bOk = VI->setupDevice(cameraID, config.cam_width, config.cam_height);

	if (bOk == true) {	
		this->cam_width = VI->getWidth(cameraID);
		this->cam_height = VI->getHeight(cameraID);
		this->fps = VI->getActualFramerate(cameraID);
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