/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 PS3EyeCamera initially contributed 2014 by Sebestyén Gábor
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PS3EyeCamera.h"
#include "CameraTool.h"
#include "ps3eye.h"


PS3EyeCamera::PS3EyeCamera(CameraConfig* cam_cfg):CameraEngine(cam_cfg) {
	cam_buffer = NULL;
	cam_cfg->driver = DRIVER_PS3EYE;
}

PS3EyeCamera::~PS3EyeCamera() {
	updateSettings();
	CameraTool::saveSettings();
	if (cam_buffer!=NULL) delete []cam_buffer;
	eye.reset();
}

int PS3EyeCamera::getDeviceCount() {
	std::vector<PS3EYECam::PS3EYERef> devices( PS3EYECam::getDevices() );
	return (int)devices.size();
}

std::vector<CameraConfig> PS3EyeCamera::getCameraConfigs() {
    
    std::vector<CameraConfig> cfg_list;
	
    int dev_count = getDeviceCount();
    if(dev_count==0) return cfg_list;
    
    if (dev_count > 0)
    {
        for (int i=0;i<dev_count;i++) {
            
            CameraConfig cam_cfg;
            CameraTool::initCameraConfig(&cam_cfg);
            
            sprintf(cam_cfg.name, "PS3Eye");
            cam_cfg.driver = DRIVER_PS3EYE;
            cam_cfg.device = i;
            cam_cfg.cam_format = FORMAT_YUYV;
            
            cam_cfg.cam_width =  320;
            cam_cfg.cam_height = 240;
			
			cam_cfg.cam_fps = 187;
			cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 150;
            cfg_list.push_back(cam_cfg);
			cam_cfg.cam_fps = 137;
			cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 125;
            cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 100;
            cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 75;
            cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 60;
            cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 50;
            cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 40;
            cfg_list.push_back(cam_cfg);
            cam_cfg.cam_fps = 30;
            cfg_list.push_back(cam_cfg);
			
			cam_cfg.cam_width  = 640;
			cam_cfg.cam_height = 480;
			
			cam_cfg.cam_fps = 60;
			cfg_list.push_back(cam_cfg);
			cam_cfg.cam_fps = 50;
			cfg_list.push_back(cam_cfg);
			cam_cfg.cam_fps = 40;
			cfg_list.push_back(cam_cfg);
			cam_cfg.cam_fps = 30;
			cfg_list.push_back(cam_cfg);
			cam_cfg.cam_fps = 15;
			cfg_list.push_back(cam_cfg);
        }
    } else {
        printf ("no PS3Eye cameras found\n");
    }
	
    return cfg_list;
}

CameraEngine* PS3EyeCamera::getCamera(CameraConfig *cam_cfg) {

	int dev_count = getDeviceCount();
	if (dev_count == 0) return NULL;
	
	if ((cam_cfg->device==SETTING_MIN) || (cam_cfg->device==SETTING_DEFAULT)) cam_cfg->device=0;
	else if (cam_cfg->device==SETTING_MAX) cam_cfg->device=dev_count-1;
	
	std::vector<CameraConfig> cfg_list = PS3EyeCamera::getCameraConfigs();
	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);
	
	if (cam_cfg->force) return new PS3EyeCamera(cam_cfg);

	int count = cfg_list.size();
	if (count > 0) {
		for (int i=0;i<count;i++) {
			
			if (cam_cfg->device != cfg_list[i].device) continue;
			
			if ((cam_cfg->cam_width >=0) && (cam_cfg->cam_width != cfg_list[i].cam_width)) continue;
			if ((cam_cfg->cam_height >=0) && (cam_cfg->cam_height != cfg_list[i].cam_height)) continue;
			if ((cam_cfg->cam_fps >=0) && (cam_cfg->cam_fps != cfg_list[i].cam_fps)) continue;

			return new PS3EyeCamera(cam_cfg);
		}
	}
	
	return NULL;
}

bool PS3EyeCamera::initCamera() {
	
	std::vector<PS3EYECam::PS3EYERef> devices( PS3EYECam::getDevices() );
	int dev_count = (int)devices.size();
	if (dev_count == 0) return false;
	if ((cfg->device<0) || (cfg->device>=dev_count)) return false;
	
	eye = devices.at(cfg->device);
	sprintf(cfg->name, "PS3Eye");
	
    // check config parameters
    if ((cfg->cam_width == SETTING_MIN || cfg->cam_width == 320) && (cfg->cam_height == SETTING_MIN || cfg->cam_height == 240)) {

        cfg->cam_width =  320;
        cfg->cam_height = 240;
        
        if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps = 187;
        else if (cfg->cam_fps==SETTING_MIN) cfg->cam_fps = 30;
        else if (cfg->cam_fps<30) cfg->cam_fps = 30;
        else if (cfg->cam_fps>187) cfg->cam_fps = 187;
        
    } else if ((cfg->cam_width == SETTING_MAX || cfg->cam_width == 640) && (cfg->cam_height == SETTING_MAX || cfg->cam_height == 480 )) {
        
        cfg->cam_width =  640;
        cfg->cam_height = 480;
        
        if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps  = 60;
        else if (cfg->cam_fps==SETTING_MIN) cfg->cam_fps  = 15;
        else if (cfg->cam_fps<15) cfg->cam_fps = 15;
        else if (cfg->cam_fps>60) cfg->cam_fps = 60;
        
	} else return false;
	
    // init camera
    eye->init( cfg->cam_width, cfg->cam_height, cfg->cam_fps );
    
    cfg->cam_width = eye->getWidth();
    cfg->cam_height = eye->getHeight();
    cfg->cam_fps = eye->getFrameRate();
    
    // do the rest
    setupFrame();
    if (cfg->frame) cam_buffer = new unsigned char[cfg->frame_width*cfg->frame_height*cfg->buf_format];
    else cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
    return true;
}

bool PS3EyeCamera::startCamera() {
    eye->start();
    applyCameraSettings();
    running = true;
    return true;
}

bool PS3EyeCamera::stopCamera() {
	running = false;
    eye->stop();
    return true;
}

bool PS3EyeCamera::stillRunning() {
    return eye->isStreaming();
}

bool PS3EyeCamera::resetCamera() {
    stopCamera();
    return startCamera();
}

bool PS3EyeCamera::closeCamera() {
    return true;
}

unsigned char* PS3EyeCamera::getFrame() {
	
    while (!eye->isNewFrame()) {
        PS3EYECam::updateDevices();
		if(!running) return NULL;
    }
	
	unsigned char *eye_buffer = (unsigned char *) eye->getLastFramePointer();
	
    if (cfg->color) {
        if (cfg->frame)
            crop_yuyv2rgb(cfg->cam_width, eye_buffer, cam_buffer);
        else
            yuyv2rgb(cfg->cam_width, cfg->cam_height, eye_buffer, cam_buffer);
            
    } else {
        
         if (cfg->frame)
             crop_yuyv2gray(cfg->cam_width, eye_buffer, cam_buffer);
         else
             yuyv2gray(cfg->cam_width, cfg->cam_height, eye_buffer, cam_buffer);
    }
    
    return cam_buffer;
}

int PS3EyeCamera::getCameraSettingStep(int mode) {
    return 1;
}

bool PS3EyeCamera::hasCameraSettingAuto(int mode) {
    
    switch (mode) {
        case GAIN:
        case WHITE:
            return true;
    }
    return false;
}

bool PS3EyeCamera::setCameraSettingAuto(int mode, bool flag) {

    switch (mode) {
        case GAIN:
        case EXPOSURE:
        case WHITE:
            eye->setAutogain(flag);
            return true;
        /*case WHITE:
            eye->setAutoWhiteBalance(flag);
            return true;*/
    }

    return false;
}

bool PS3EyeCamera::getCameraSettingAuto(int mode) {

    switch (mode) {
        case WHITE:
        case GAIN:
        case EXPOSURE:
            return eye->getAutogain();
        /*case WHITE:
            return eye->getAutoWhiteBalance();*/
    }
    
    return false;
}

bool PS3EyeCamera::hasCameraSetting(int mode) {
    
    switch (mode) {
        case GAIN:
        case AUTO_GAIN:
        case AUTO_EXPOSURE:
        case AUTO_WHITE:
        case EXPOSURE:
        case SHARPNESS:
        case BRIGHTNESS:
        case CONTRAST:
        case COLOR_HUE:
        case COLOR_RED:
        case COLOR_BLUE:
        case COLOR_GREEN:
            return true;
        default:
            return false;
    }
    return false;
}

bool PS3EyeCamera::setCameraSetting(int mode, int value) {

    switch (mode) {
        case AUTO_GAIN:
        case AUTO_EXPOSURE:
        case AUTO_WHITE:
            eye->setAutogain(value);
            return true;
        case GAIN:
			eye->setAutogain(false);
            eye->setGain(value);
            return true;
        case EXPOSURE:
			eye->setExposure(value);
            return true;
        case SHARPNESS:
            eye->setSharpness(value);
            return true;
        case BRIGHTNESS:
            eye->setBrightness(value);
            return true;
        case CONTRAST:
            eye->setContrast(value);
            return true;
        case COLOR_HUE:
            eye->setHue(value);
            return true;
        case COLOR_RED:
            eye->setRedBalance(value);
            return true;
        case COLOR_BLUE:
            eye->setBlueBalance(value);
            return true;
        case COLOR_GREEN:
            eye->setGreenBalance(value);
            return true;
    }
    return false;
}

int PS3EyeCamera::getCameraSetting(int mode) {

    switch (mode) {
        case GAIN:
            return eye->getGain();
        case AUTO_GAIN:
        case AUTO_EXPOSURE:
        case AUTO_WHITE:
            return eye->getAutogain();
        case EXPOSURE:
            return eye->getExposure();
        case SHARPNESS:
            return eye->getSharpness();
        case BRIGHTNESS:
            return eye->getBrightness();
        case CONTRAST:
            return eye->getContrast();
        case COLOR_HUE:
            return eye->getHue();
        case COLOR_RED:
            return eye->getRedBalance();
        case COLOR_BLUE:
            return eye->getBlueBalance();
        case COLOR_GREEN:
            return eye->getGreenBalance();
    }
    return 0;
}

int PS3EyeCamera::getMaxCameraSetting(int mode) {
    
    switch (mode) {
        case AUTO_GAIN:
        case AUTO_EXPOSURE:
        case AUTO_WHITE:
            return 1;
        case GAIN:
            return 63;
        case EXPOSURE:
        case SHARPNESS:
        case BRIGHTNESS:
        case CONTRAST:
        case COLOR_HUE:
        case COLOR_RED:
        case COLOR_BLUE:
        case COLOR_GREEN:
            return 255;
    }
    return 0;
}


int PS3EyeCamera::getMinCameraSetting(int mode) {
    return 0;
}

bool PS3EyeCamera::setDefaultCameraSetting(int mode) {
    switch (mode) {
        case GAIN:
            eye->setGain(getDefaultCameraSetting(mode));
            return true;
        case AUTO_GAIN:
        case AUTO_WHITE:
        case AUTO_EXPOSURE:
            eye->setAutogain(getDefaultCameraSetting(mode));
            return true;
        /*case AUTO_WHITE:
            eye->setAutoWhiteBalance(getDefaultCameraSetting(mode));
            return true;*/
        case EXPOSURE:
            eye->setExposure(getDefaultCameraSetting(mode));
            return true;
        case SHARPNESS:
            eye->setSharpness(getDefaultCameraSetting(mode));
            return true;
        case BRIGHTNESS:
            eye->setBrightness(getDefaultCameraSetting(mode));
            return true;
        case CONTRAST:
            eye->setContrast(getDefaultCameraSetting(mode));
            return true;
        case COLOR_HUE:
            eye->setHue(getDefaultCameraSetting(mode));
            return true;
        case COLOR_RED:
            eye->setRedBalance(getDefaultCameraSetting(mode));
            return true;
        case COLOR_BLUE:
            eye->setBlueBalance(getDefaultCameraSetting(mode));
            return true;
        case COLOR_GREEN:
            eye->setGreenBalance(getDefaultCameraSetting(mode));
            return true;
        }
    return false;
}

int PS3EyeCamera::getDefaultCameraSetting(int mode) {
    switch (mode) {
        case AUTO_GAIN:
        case AUTO_EXPOSURE:
        case AUTO_WHITE:
            return 1;
        case EXPOSURE:
            return 64;
        case BRIGHTNESS:
        case CONTRAST:
        case SHARPNESS:
        case GAIN:
            return 32;
        case COLOR_HUE:
        case COLOR_RED:
        case COLOR_BLUE:
        case COLOR_GREEN:
            return 128;
    }
    return 0;
}
