/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>

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

#include <list>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <limits.h>
#include <math.h>

#include "FrameProcessor.h"
#include "UserInterface.h"
#include "tinyxml2.h"

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#elif LINUX
#include <unistd.h>
#endif

#define SAT(c) \
if (c & (~255)) { if (c < 0) c = 0; else c = 255; }
#define HBT(x) (unsigned char)((x)>>8)

#define SETTING_DEFAULT -100000
#define SETTING_AUTO    -200000
#define SETTING_MIN     -300000
#define SETTING_MAX     -400000
#define SETTING_OFF     -500000

#define FORMAT_UNSUPPORTED  -1
#define FORMAT_UNKNOWN  0
#define FORMAT_GRAY     1
#define FORMAT_GRAY16   2
#define FORMAT_RGB      3
#define FORMAT_RGB16    4
#define FORMAT_GRAY16S  5
#define FORMAT_RGB16S   6
#define FORMAT_RAW8		7
#define FORMAT_RAW16    8
#define FORMAT_RGBA		9
#define FORMAT_YUYV    10
#define FORMAT_UYVY    11
#define FORMAT_YUV411  12
#define FORMAT_YUV444  13
#define FORMAT_420P    14
#define FORMAT_410P    15
#define FORMAT_YVYU    16
#define FORMAT_YUV211  17
#define FORMAT_JPEG    20
#define FORMAT_MJPEG   21
#define FORMAT_MPEG    22
#define FORMAT_MPEG2   23
#define FORMAT_MPEG4   24
#define FORMAT_H263    25
#define FORMAT_H264    26
#define FORMAT_DVPAL   30
#define FORMAT_DVNTSC  31
#define FORMAT_MAX     31

extern const char* fstr[];
extern const char* dstr[];

#define DRIVER_DEFAULT  0
#define DRIVER_DC1394   1
#define DRIVER_PS3EYE   2
#define DRIVER_RASPI    3
#define DRIVER_UVCCAM   4
#define DRIVER_FILE    10
#define DRIVER_FOLDER  11

#define VALUE_INCREASE   79
#define VALUE_DECREASE   80
#define SETTING_NEXT     81
#define SETTING_PREVIOUS 82

enum CameraSetting { BRIGHTNESS, CONTRAST, SHARPNESS, AUTO_GAIN, GAIN, AUTO_EXPOSURE, EXPOSURE, SHUTTER, AUTO_FOCUS, FOCUS, AUTO_WHITE, WHITE, GAMMA, POWERLINE, BACKLIGHT, SATURATION, AUTO_HUE, COLOR_HUE, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
#define MODE_MIN BRIGHTNESS
#define MODE_MAX COLOR_BLUE

struct CameraConfig {

    char path[256];

    int driver;
    int device;

	char name[256];
    char src[256];

    bool color;
    bool frame;

    int cam_format;
    int src_format;
    int buf_format;

    int cam_width;
    int cam_height;
    float cam_fps;

    int frame_width;
    int frame_height;
    int frame_xoff;
    int frame_yoff;
    int frame_mode;

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

	int saturation;
    int hue;
    int red;
    int blue;
    int green;
	
	bool force;

    bool operator < (const CameraConfig& c) const {

       //if (device < c.device) return true;
       //if (cam_format < c.cam_format) return true;

       if (cam_width > c.cam_width || (cam_width == c.cam_width && cam_height < c.cam_height))
                return true;

       if (cam_width == c.cam_width && cam_height == c.cam_height) {
                return (cam_fps > c.cam_fps);
       } else return false;

    }
};

class CameraEngine
{
public:

    CameraEngine(CameraConfig *cam_cfg) {
        cfg = cam_cfg;
        settingsDialog=false;

        if (cfg->color) cfg->buf_format=FORMAT_RGB;
        else cfg->buf_format=FORMAT_GRAY;
    }

    virtual ~CameraEngine() {};

    virtual bool initCamera() = 0;
    virtual bool startCamera() = 0;
    virtual unsigned char* getFrame() = 0;
    virtual bool stopCamera() = 0;
    virtual bool resetCamera() = 0;
    virtual bool closeCamera() = 0;
    virtual bool stillRunning() = 0;

    void printInfo();
    static void setMinMaxConfig(CameraConfig *cam_cfg, std::vector<CameraConfig> cfg_list);

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
    int getFormat() { return cfg->buf_format; }
    char* getName() { return cfg->name; }

protected:

    CameraConfig *cfg;

    unsigned char* frm_buffer;
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

	int default_saturation;
    int default_hue;
    int default_red;
    int default_blue;
    int default_green;

    int ctrl_min;
    int ctrl_max;
    int ctrl_val;
};
#endif
