/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>
 PS3EyeCamera contributed 2014 by Sebestyén Gábor
 
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
#include "ps3eye.h"

PS3EyeCamera::PS3EyeCamera(const char* config_file):CameraEngine(config_file)
{
    cam_buffer = NULL;
}

PS3EyeCamera::~PS3EyeCamera() {
    if (cam_buffer!=NULL) delete []cam_buffer;
}

void PS3EyeCamera::listDevices() {
    using namespace ps3eye;
    
    // list out the devices
    std::vector<PS3EYECam::PS3EYERef> devices( PS3EYECam::getDevices() );
    
    int count = (int)devices.size();
    if (count > 0)
    {
        //PS3EYECam::PS3EYERef inst = devices.at(0);
        if (count==1) printf("1 PS3Eye camera found:\n");
        else printf("%d PS3Eye cameras found:\n",count);
        
        for (int i=0;i<count;i++) {
            printf("\t%d: PS3Eye%d\n",i,i);
            printf("\t\tformat: YUV422\n");
            printf("\t\t\t320x240 150|125|100|75|60|50|40|30 fps\n");
            printf("\t\t\t640x480 60|50|40|30|15 fps\n");
        }
        
    } else {
        printf ("no PS3Eye cameras found\n");
    }
}

bool PS3EyeCamera::findCamera() {
    using namespace ps3eye;

    readSettings();

    // list out the devices
    std::vector<PS3EYECam::PS3EYERef> devices( PS3EYECam::getDevices() );
    
    if (devices.size() > 0)
    {
        cameraID = config.device;
        if (cameraID>=devices.size()) return false;
        else if (cameraID < 0) cameraID = 0;
        
        eye = devices.at(config.device);
        sprintf(cameraName, "PS3Eye");
    } else printf("no PS3Eye cameras found\n");

    return eye != NULL;
}

bool PS3EyeCamera::initCamera() {
    // check config parameters
    if (config.cam_width == SETTING_MIN || config.cam_width == 320 || config.cam_height == SETTING_MIN || config.cam_height == 240) {

        config.cam_width = cam_width = 320;
        config.cam_height = cam_height =  240;
        
        if (config.cam_fps==SETTING_MAX) config.cam_fps = 150;
        else if (config.cam_fps==SETTING_MIN) config.cam_fps = 30;
        else if (config.cam_fps<30) config.cam_fps = fps = 30;
        else if (config.cam_fps>150) config.cam_fps = fps = 150;
        else fps = config.cam_fps;
        
    } else if (config.cam_width == SETTING_MAX || config.cam_width == 640 || config.cam_height == SETTING_MAX || config.cam_height == 480 ) {
        
        config.cam_width =  cam_width =  640;
        config.cam_height = cam_height = 480;
        
        if (config.cam_fps==SETTING_MAX) config.cam_fps = fps = 60;
        else if (config.cam_fps==SETTING_MIN) config.cam_fps = fps = 15;
        else if (config.cam_fps<15) config.cam_fps = fps = 15;
        else if (config.cam_fps>60) config.cam_fps = fps = 60;
        else fps = config.cam_fps;
        
    } else {
        config.cam_width = cam_width = 640;
        config.cam_height = cam_height = 480;
        config.cam_fps = fps = 60;
    }
    
    // init camera
    eye->init( config.cam_width, config.cam_height, config.cam_fps );
    config.cam_fps = fps = eye->getFrameRate();
    
    applyCameraSettings();
    
    // do the rest
    setupFrame();
    if (config.frame) cam_buffer = new unsigned char[cam_width*cam_height*bytes];
    else cam_buffer = new unsigned char[frame_width*frame_height*bytes];
    return true;
}

bool PS3EyeCamera::startCamera() {
    eye->start();
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
    // TODO
    return false;
}

bool PS3EyeCamera::closeCamera() {
    return true;
}

unsigned char*  PS3EyeCamera::getFrame() {
    using namespace ps3eye;

    
    while (!eye->isNewFrame()) {
        PS3EYECam::updateDevices();
    }
    
    if (colour) {
        yuyv2rgb(cam_width, cam_height, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
    } else {
        
         if (config.frame)
             crop_yuyv2gray(cam_width, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
         else
             yuyv2gray(cam_width, cam_height, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
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
            eye->setAutogain(flag);
            return true;
        case WHITE:
            eye->setAutoWhiteBalance(flag);
            return true;
    }

    return false;
}

bool PS3EyeCamera::getCameraSettingAuto(int mode) {

    switch (mode) {
        case GAIN:
            return eye->getAutogain();
        case WHITE:
            return eye->getAutoWhiteBalance();
    }
    
    return false;
}

bool PS3EyeCamera::hasCameraSetting(int mode) {
    
    switch (mode) {
        case GAIN:
        case AUTO_GAIN:
        case EXPOSURE:
        case SHARPNESS:
        case BRIGHTNESS:
        case CONTRAST:
        case AUTO_WHITE:
        case COLOR_HUE:
        case COLOR_RED:
        case COLOR_BLUE:
        case COLOR_GREEN:
            return true;
    }
    return false;
}

bool PS3EyeCamera::setCameraSetting(int mode, int value) {

    switch (mode) {
        case GAIN:
            _gain = value;
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
            eye->setBrightness(getDefaultCameraSetting(mode));
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
        return true;    }
    return false;
}

int PS3EyeCamera::getDefaultCameraSetting(int mode) {
    switch (mode) {
        case GAIN:
            return 0;
        case EXPOSURE:
            return 128;
        case SHARPNESS:
            return 32;
        case BRIGHTNESS:
            return 32;
        case CONTRAST:
            return 64;
        case COLOR_HUE:
            return 128;
        case COLOR_RED:
            return 128;
        case COLOR_BLUE:
            return 128;
        case COLOR_GREEN:
            return 128;
    }
    return 0;
}
