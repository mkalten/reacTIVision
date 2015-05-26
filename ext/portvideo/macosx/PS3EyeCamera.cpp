/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>
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

std::vector<CameraConfig> PS3EyeCamera::findDevices() {
    
    std::vector<CameraConfig> cfg_list;
    std::vector<PS3EYECam::PS3EYERef> devices( PS3EYECam::getDevices() );
    
    int count = (int)devices.size();
    if (count==1) printf("1 PS3Eye camera found:\n");
    else printf("%d PS3Eye cameras found:\n",count);
    
    if (count > 0)
    {
        for (int i=0;i<count;i++) {
            
            CameraConfig cam_cfg;
            CameraTool::initCameraConfig(&cam_cfg);
            
            sprintf(cam_cfg.name, "PS3Eye");
            cam_cfg.driver = DRIVER_PS3EYE;
            cam_cfg.device = i;
            cam_cfg.cam_format = FORMAT_YUYV;
            cam_cfg.src_format = FORMAT_YUYV;
            
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
            
            cam_cfg.cam_width =  320;
            cam_cfg.cam_height = 240;
            
            cam_cfg.cam_fps = 150;
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
        }
    } else {
        printf ("no PS3Eye cameras found\n");
    }
    
    return cfg_list;
}

void PS3EyeCamera::listDevices() {
    
    std::vector<CameraConfig> cfg_list = PS3EyeCamera::findDevices();
    
    int device = INT_MIN;
    int width = INT_MAX;
    int height = INT_MAX;
    int fps = INT_MAX;
    
    int count = cfg_list.size();
    if (count > 0)
    {
        for (int i=0;i<count;i++) {
            
            if (cfg_list[i].device > device) {
                printf("\t%d: PS3Eye\n",i);
                printf("\t\tformat: YUV422\n");
                device = cfg_list[i].device;
                if (device>0) printf(" fps\n");
            }
            
            if ((cfg_list[i].cam_width < width) && (cfg_list[i].cam_height < height)) {
                printf("\t\t\t%dx%d ",cfg_list[i].cam_width,cfg_list[i].cam_height);
                width = cfg_list[i].cam_width;
                height = cfg_list[i].cam_height;
                fps = INT_MAX;
            }
            
            if (cfg_list[i].cam_fps < fps) {
                printf("%d|",(int)cfg_list[i].cam_fps);
                fps = cfg_list[i].cam_fps;
            }
        }
        
    }
}

bool PS3EyeCamera::findCamera() {

    // list out the devices
    std::vector<PS3EYECam::PS3EYERef> devices( PS3EYECam::getDevices() );
    
    if ((int)devices.size() > 0)
    {
        
        if (devices.size() == 1) printf("1 PS3Eye camera found\n");
        else printf("%d PS3Eye cameras found\n", (int)devices.size());
        
        if (cfg->device >= devices.size()) cfg->device = devices.size()-1;
        else if (cfg->device < 0) cfg->device = 0;
        
        eye = devices.at(cfg->device);
        sprintf(cfg->name, "PS3Eye");
    } else {
        printf("no PS3Eye cameras found\n");
        return false;
    }

    return eye != NULL;
}

bool PS3EyeCamera::initCamera() {
    // check config parameters
    if (cfg->cam_width == SETTING_MIN || cfg->cam_width == 320 || cfg->cam_height == SETTING_MIN || cfg->cam_height == 240) {

        cfg->cam_width =  320;
        cfg->cam_height = 240;
        
        if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps = max_fps = 150;
        else if (cfg->cam_fps==SETTING_MIN) cfg->cam_fps = min_fps = 30;
        else if (cfg->cam_fps<30) cfg->cam_fps = 30;
        else if (cfg->cam_fps>150) cfg->cam_fps = 150;
        
    } else if (cfg->cam_width == SETTING_MAX || cfg->cam_width == 640 || cfg->cam_height == SETTING_MAX || cfg->cam_height == 480 ) {
        
        cfg->cam_width =  640;
        cfg->cam_height = 480;
        
        if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps = max_fps = 60;
        else if (cfg->cam_fps==SETTING_MIN) cfg->cam_fps = min_fps = 15;
        else if (cfg->cam_fps<15) cfg->cam_fps = 15;
        else if (cfg->cam_fps>60) cfg->cam_fps = 60;
        
    } else {
        cfg->cam_width =  640;
        cfg->cam_height = 480;
        cfg->cam_fps = 60;
    }
    
    // init camera
    eye->init( cfg->cam_width, cfg->cam_height, cfg->cam_fps );
    
    cfg->cam_width = eye->getWidth();
    cfg->cam_height = eye->getHeight();
    cfg->cam_fps = eye->getFrameRate();
    
    // do the rest
    setupFrame();
    if (cfg->frame) cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
    else cam_buffer = new unsigned char[cfg->frame_width*cfg->frame_height*cfg->buf_format];
    return true;
}

bool PS3EyeCamera::startCamera() {
    eye->start();
    applyCameraSettings();
    running = true;
    return true;
}

bool PS3EyeCamera::stopCamera() {
    eye->stop();
    running = false;
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

unsigned char*  PS3EyeCamera::getFrame() {
    
    while (!eye->isNewFrame()) {
        PS3EYECam::updateDevices();
    }
    
    if (cfg->color) {
        if (cfg->frame)
            crop_yuyv2rgb(cfg->cam_width, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
        else
            yuyv2rgb(cfg->cam_width, cfg->cam_height, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
            
    } else {
        
         if (cfg->frame)
             crop_yuyv2gray(cfg->cam_width, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
         else
             yuyv2gray(cfg->cam_width, cfg->cam_height, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
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
