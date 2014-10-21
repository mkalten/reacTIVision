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

#ifndef CAMERAENGINE_H
#define CAMERAENGINE_H

#include <fstream>
#include <iostream>
#include <limits.h>
#include "tinyxml.h"
#include <math.h>
#include <SDL.h>
#include "FontTool.h"

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#endif

#define SAT(c) \
        if (c & (~255)) { if (c < 0) c = 0; else c = 255; }
#define SETTING_DEFAULT -1
#define SETTING_AUTO -2
#define SETTING_MIN -3
#define SETTING_MAX -4

class CameraEngine
{
public:
	CameraEngine(const char* cfg) {
		config_file = cfg;
		settingsDialog=false;
        crop_frame=false;
	}

	virtual ~CameraEngine() { };

	virtual bool findCamera() = 0;
	virtual bool initCamera() = 0;
	virtual bool startCamera() = 0;
	virtual unsigned char* getFrame() = 0;
	virtual bool stopCamera() = 0;
	virtual bool resetCamera() = 0;
	virtual bool closeCamera() = 0;
	virtual bool stillRunning() = 0;

	enum CameraSetting { BRIGHTNESS, CONTRAST, GAIN, SHUTTER, EXPOSURE, SHARPNESS, FOCUS, GAMMA };
	enum CropMode { CROP_NONE, CROP_RECT, CROP_SQUARE, CROP_WIDE};

	virtual int getCameraSettingStep(int mode) = 0;
	virtual int getMinCameraSetting(int mode) = 0;
	virtual int getMaxCameraSetting(int mode) = 0;
	virtual int getCameraSetting(int mode) = 0;
	virtual bool setCameraSetting(int mode, int value) = 0;
	virtual bool setCameraSettingAuto(int mode, bool flag) = 0;
	virtual bool getCameraSettingAuto(int mode) = 0;
	virtual bool setDefaultCameraSetting(int mode) = 0;
	virtual int getDefaultCameraSetting(int mode) = 0;

	virtual void showSettingsDialog();
	virtual void control(int key);
	virtual void drawGUI(SDL_Surface *display);

	int getId() { return cameraID; }
	int getFps() { return (int)floor(fps+0.5f); }
	int getWidth() { return frame_width; }
	int getHeight() { return frame_height; }
	char* getName() { return cameraName; }

protected:
	int cameraID;
	char cameraName[256];

	struct portvideo_settings {
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
			int gain;
			int shutter;
			int exposure;
			int contrast;
			int sharpness;
			int focus;
			int gamma;
	};
	portvideo_settings config;
	const char* config_file;
	char* image_file;
	#ifdef __APPLE__
	char full_path[1024];
	#endif

    bool crop_frame;
    unsigned char* crop_buffer;
    int crop_xoff, crop_yoff;

	unsigned char* cam_buffer;
	int bytes;

	int cam_width, cam_height;
    int frame_width, frame_height;
	float fps;
	bool colour;

	int timeout;
	int lost_frames;

	bool running;
	bool settingsDialog;
	int currentCameraSetting;

	void uyvy2gray(int width, int height, unsigned char *src, unsigned char *dest);
	void yuyv2gray(int width, int height, unsigned char *src, unsigned char *dest);
	void uyvy2rgb(int width, int height, unsigned char *src, unsigned char *dest);
    void rgb2gray(int width, int height, unsigned char *src, unsigned char *dest);

    void readSettings();
    void saveSettings();
    void applyCameraSettings();
    void applyCameraSetting(int mode, int value);
    void updateSettings();

    void setupFrame();
    void cropFrame(unsigned char *src, unsigned char *dest);

    int default_brightness;
    int default_contrast;
    int default_gain;
    int default_shutter;
    int default_exposure;
    int default_sharpness;
    int default_focus;
    int default_gamma;
};
#endif
