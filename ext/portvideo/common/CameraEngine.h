/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

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

#ifndef CAMERAENGINE_H
#define CAMERAENGINE_H

#include <fstream>
#include <iostream>
#include <limits.h>
#include "tinyxml2.h"
#include <math.h>
#include "FrameProcessor.h"
#include "UserInterface.h"

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#elif LINUX
#include <unistd.h>
#endif

#define SAT(c) \
if (c & (~255)) { if (c < 0) c = 0; else c = 255; }
#define HIBYTE(x) (unsigned char)((x)>>8)

#define SETTING_DEFAULT -1
#define SETTING_AUTO    -2
#define SETTING_MIN     -3
#define SETTING_MAX     -4
#define SETTING_OFF     -5

#define DRIVER_DEFAULT 0
#define DRIVER_DC1394  1
#define DRIVER_PS3EYE  2
#define DRIVER_RASPI   3
#define DRIVER_UVCCAM  4
#define DRIVER_FILE    10
#define DRIVER_FOLDER  11

#define VALUE_UP 79
#define VALUE_DOWN 80
#define SETTING_NEXT 81
#define SETTING_PREVIOUS 82

enum CameraSetting { BRIGHTNESS, CONTRAST, SHARPNESS, AUTO_GAIN, GAIN, AUTO_EXPOSURE, EXPOSURE, SHUTTER, AUTO_FOCUS, FOCUS, AUTO_WHITE, WHITE, GAMMA, POWERLINE, BACKLIGHT, AUTO_HUE, COLOR_HUE, COLOR_RED, COLOR_GREEN, COLOR_BLUE };

struct CameraConfig {
    int driver;
    int device;
    char file[255];
    char folder[255];
    
    bool color;
    bool compress;
    bool frame;
    
    int cam_width;
    int cam_height;
    float cam_fps;
    
    int frame_xoff;
    int frame_yoff;
    int frame_width;
    int frame_height;
    
    int brightness;
    int contrast;
    int sharpness;
    
    int gain;
    int shutter;
    int exposure;
    int focus;
    int gamma;
    int white;
    int powerline;
    int backlight;
    
    int hue;
    int red;
    int blue;
    int green;
};

class CameraEngine
{
public:
    
    CameraEngine(CameraConfig *cam_cfg) {
        cfg = cam_cfg;
        settingsDialog=false;
        
        if (cfg->color) bytes = 3;
        else bytes = 1;
    }

    virtual ~CameraEngine() {};

    virtual bool findCamera() = 0;
    virtual bool initCamera() = 0;
    virtual bool startCamera() = 0;
    virtual unsigned char* getFrame() = 0;
    virtual bool stopCamera() = 0;
    virtual bool resetCamera() = 0;
    virtual bool closeCamera() = 0;
    virtual bool stillRunning() = 0;

    virtual int getCameraSettingStep(int mode) = 0;
    virtual int getMinCameraSetting(int mode) = 0;
    virtual int getMaxCameraSetting(int mode) = 0;
    virtual int getCameraSetting(int mode) = 0;
    virtual bool setCameraSetting(int mode, int value) = 0;
    virtual bool setCameraSettingAuto(int mode, bool flag) = 0;
    virtual bool getCameraSettingAuto(int mode) = 0;
    virtual bool setDefaultCameraSetting(int mode) = 0;
    virtual int getDefaultCameraSetting(int mode) = 0;
    virtual bool hasCameraSetting(int mode) = 0;
    virtual bool hasCameraSettingAuto(int mode) = 0;

    virtual bool showSettingsDialog(bool lock);
    virtual void control(unsigned char key);
    virtual void showInterface(UserInterface *uiface);

    int getId() { return cfg->device; }
    int getFps() { return (int)floor(cfg->cam_fps+0.5f); }
    int getWidth() { return cfg->frame_width; }
    int getHeight() { return cfg->frame_height; }
    bool getColour() { return cfg->color; }
    char* getName() { return cameraName; }

protected:

    char cameraName[256];
    CameraConfig *cfg;

    int bytes;
    unsigned char* crop_buffer;
    unsigned char* cam_buffer;

    int lost_frames, timeout;

    bool running;
    bool settingsDialog;
    int currentCameraSetting;

	void crop(int width, int height, unsigned char *src, unsigned char *dest, int bytes);
	void flip(int width, int height, unsigned char *src, unsigned char *dest, int bytes);
	void flip_crop(int width, int height, unsigned char *src, unsigned char *dest, int bytes);

    void rgb2gray(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_rgb2gray(int width, unsigned char *src, unsigned char *dest);
	void flip_rgb2gray(int width, int height, unsigned char *src, unsigned char *dest);
    void flip_crop_rgb2gray(int width, unsigned char *src, unsigned char *dest);

    void uyvy2gray(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_uyvy2gray(int width, unsigned char *src, unsigned char *dest);
    void yuyv2gray(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_yuyv2gray(int width, unsigned char *src, unsigned char *dest);
    void yuv2gray(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_yuv2gray(int width, unsigned char *src, unsigned char *dest);

    void gray2rgb(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_gray2rgb(int width, unsigned char *src, unsigned char *dest);
    void uyvy2rgb(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_uyvy2rgb(int width, unsigned char *src, unsigned char *dest);
    void yuyv2rgb(int width, int height, unsigned char *src, unsigned char *dest);
    void crop_yuyv2rgb(int width, unsigned char *src, unsigned char *dest);

    void resetCameraSettings();
    void applyCameraSettings();
    void applyCameraSetting(int mode, int value);
    void updateSettings();
    int updateSetting(int mode);

    void setupFrame();
    //void cropFrame(unsigned char *src, unsigned char *dest, int bytes);

    int max_width, min_width;
    int max_height, min_height;
    int max_fps, min_fps;
    
    int default_brightness;
    int default_contrast;
    int default_sharpness;

    int default_gain;
    int default_shutter;
    int default_exposure;
    int default_focus;
    int default_gamma;
    int default_powerline;
    int default_white;
    int default_backlight;

    int default_hue;
    int default_red;
    int default_blue;
    int default_green;

    int ctrl_min;
    int ctrl_max;
    int ctrl_val;
};
#endif
