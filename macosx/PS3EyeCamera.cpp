/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 PS3EyeCamera Copyright (C) 2014 Sebestyén Gábor
 
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


#include "PS3EyeCamera.h"
#include "ps3eye.h"

#define _saturate(v) static_cast<uint8_t>(static_cast<uint32_t>(v) <= 0xff ? v : v > 0 ? 0xff : 0)

static void yuv422_to_gray(const uint8_t *yuv_src, const int stride, uint8_t *dst, const int width, const int height)
{
    int j, i;
    
    for (j = 0; j < height; j++, yuv_src += stride)
    {
        uint8_t* row = dst + width * j; // 4 channels
        
        for (i = 0; i < 2*width; i += 4, row += 2)
        {
            row[0] = _saturate(yuv_src[i]);
            row[1] = _saturate(yuv_src[i + 2]);
        }
    }
}

PS3EyeCamera::PS3EyeCamera(const char* config_file):CameraEngine(config_file)
{
    cam_buffer = NULL;
	_gain = 20;
}

PS3EyeCamera::~PS3EyeCamera() {
    if (cam_buffer!=NULL) delete []cam_buffer;
	eye.reset();
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
            printf("\t\t\t320x240 187|150|125|100|75|60|50|40|30 fps\n");
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
        if (cameraID < 0) {
            cameraID = 0;
        } else if ((unsigned int)cameraID>=devices.size()) {
            return false;
        }
        
        eye = devices.at(config.device);
        sprintf(cameraName, "PS3Eye");
    } else {
        fprintf (stderr, "no PS3Eye cameras found\n");
    }

    return eye != NULL;
}

bool PS3EyeCamera::initCamera() {
    // check config parameters
    if (config.cam_width == SETTING_MIN || config.cam_width == 320 || config.cam_height == SETTING_MIN || config.cam_height == 240) {
        
        config.cam_width = 320;
        config.cam_height = 240;
        
        if (config.cam_fps==SETTING_MAX) config.cam_fps = 187;
        else if (config.cam_fps==SETTING_MIN) config.cam_fps = 30;
            
    } else if (config.cam_width == SETTING_MAX || config.cam_width == 640 || config.cam_height == SETTING_MAX || config.cam_height == 480 ) {
        
        config.cam_width = 640;
        config.cam_height = 480;
        
        if (config.cam_fps==SETTING_MAX) config.cam_fps = 60;
        else if (config.cam_fps==SETTING_MIN) config.cam_fps = 15;
        
    } else {
        // NOT SUPPORTED CONFIGURATION
        return false;
    }
    
    // init camera
    eye->init( config.cam_width, config.cam_height, config.cam_fps );
    fps = eye->getFrameRate();
    cam_buffer = new unsigned char[cam_width*cam_height*bytes];
    
    // do the rest
    setupFrame();
    return true;
}

bool PS3EyeCamera::startCamera() {
    eye->start();
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
		if(!running) return NULL;
    }
    
    //if (eye->isNewFrame()) {
    if (colour) {
        uyvy2rgb(cam_width, cam_height, (unsigned char *) eye->getLastFramePointer(), cam_buffer);
    } else {
        yuv422_to_gray((unsigned char *) eye->getLastFramePointer(), eye->getRowBytes(), cam_buffer, cam_width, cam_height);
    } //} else return NULL;
        
    return cam_buffer;
}

int PS3EyeCamera::getCameraSettingStep(int mode) {
    return 1;
}

bool PS3EyeCamera::setCameraSettingAuto(int mode, bool flag) {
    // supported: gain, awb = Automatic White Balance?

    CameraSetting   _m = (CameraSetting)mode;
    if (_m == GAIN) {
        eye->setAutogain(flag);
        return true;
    } /*else if (_m == EYE_AWB) {
        eye->setAutoWhiteBalance(flag);
    }*/

    return false;
}

bool PS3EyeCamera::getCameraSettingAuto(int mode) {
    CameraSetting   _m = (CameraSetting)mode;

    // supported: gain, awb = Automatic White Balance?

    return _m == GAIN;// || _m == EYE_AWB;
}

bool PS3EyeCamera::setCameraSetting(int mode, int value) {
    CameraSetting _m = (CameraSetting) mode;
    switch (_m) {
        case CameraEngine::GAIN:
            _gain = value;
            eye->setGain(value);
            return true;
        case CameraEngine::EXPOSURE:
            eye->setExposure(value);
            return true;
        case CameraEngine::SHARPNESS:
            eye->setSharpness(value);
            return true;
        case CameraEngine::BRIGHTNESS:
            eye->setBrightness(value);
            return true;
        case CameraEngine::CONTRAST:
            eye->setContrast(value);
            return true;

        // Not supported by PS3 Eye
        case CameraEngine::SHUTTER:
        case CameraEngine::FOCUS:
        case CameraEngine::GAMMA:
            return false;
            
        /*case CameraEngine::EYE_HUE:
            eye->setHue(value);
            return true;
        case CameraEngine::EYE_BLUE_BLC:
            eye->setBlueBalance(value);
            return true;
        case CameraEngine::EYE_RED_BLC:
            eye->setRedBalance(value);
            return true;*/
    }
    return false;
}

int PS3EyeCamera::getCameraSetting(int mode) {
    CameraSetting _m = (CameraSetting) mode;
    switch (_m) {
        case CameraEngine::GAIN:
            return eye->getGain();
        case CameraEngine::EXPOSURE:
            return eye->getExposure();
        case CameraEngine::SHARPNESS:
            return eye->getSharpness();
        case CameraEngine::BRIGHTNESS:
            return eye->getBrightness();
        case CameraEngine::CONTRAST:
            return eye->getContrast();
            
        // Not supported by PS3 Eye
        case CameraEngine::SHUTTER:
        case CameraEngine::FOCUS:
        case CameraEngine::GAMMA:
            return 0;
            
        /*case CameraEngine::EYE_HUE:
            return eye->getHue();
        case CameraEngine::EYE_RED_BLC:
            return eye->getRedBalance();
        case CameraEngine::EYE_BLUE_BLC:
            return eye->getBlueBalance();*/
    }
    return 0;
}

int PS3EyeCamera::getMaxCameraSetting(int mode) {
    CameraSetting _m = (CameraSetting) mode;
    
    switch (_m) {
        case CameraEngine::GAIN:
            return 63;
        case CameraEngine::EXPOSURE:
        case CameraEngine::SHARPNESS:
        case CameraEngine::BRIGHTNESS:
        case CameraEngine::CONTRAST:
        /*case CameraEngine::EYE_HUE:
        case CameraEngine::EYE_BLUE_BLC:
        case CameraEngine::EYE_RED_BLC:*/
            return 255;
        default:
            return 100;
    }
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
			eye->setContrast(getDefaultCameraSetting(mode));
			return true;
	}
	return false;
}

int PS3EyeCamera::getDefaultCameraSetting(int mode) {
	switch (mode) {
		case EXPOSURE:
			return 64;
		case BRIGHTNESS:
		case CONTRAST:
		case SHARPNESS:
		case GAIN:
			return 32;
	}
	return 0;
}
