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


#ifndef VISIONENGINE_H
#define VISIONENGINE_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#endif

#include <sys/stat.h>
#include <time.h>

#define WIDTH 640
#define HEIGHT 480

#include "Main.h"
#include "CameraTool.h"
#include "RingBuffer.h"
#include "UserInterface.h"

class VisionEngine
{

public:
	VisionEngine(const char* name, const char * camera_config);
	~VisionEngine() {};

	void start();
	void stop();
    void event(int key);
    void pause(bool pause);

	bool running_;
	bool error_;
	bool pause_;
	bool calibrate_;
	bool help_;
    bool display_lock_;
    unsigned int current_fps_;

    CameraEngine *camera_;
	RingBuffer *ringBuffer;

    void setInterface (UserInterface *uiface);
	void addFrameProcessor(FrameProcessor *fp);
	void removeFrameProcessor(FrameProcessor *fp);

	long framenumber_;



protected:
	void initFrameProcessors();

	void allocateBuffers();
	void freeBuffers();

	void mainLoop();
	void endLoop();
    
    void startThread();
    void stopThread();
    
    void setupCamera(const char *camera_config);
    void teardownCamera();

	unsigned char* sourceBuffer_;
	unsigned char* destBuffer_;
    unsigned char* displayBuffer_;
    
    long currentTime() {
        time_t currentTime;
        time(&currentTime);
        return (long) currentTime;
    }
    
#ifndef NDEBUG
    void saveBuffer(unsigned char* buffer);
    bool recording_;
#endif

    UserInterface *interface_;

private:
	long frames_;
  	long lastTime_;
	unsigned int cameraTime_, processingTime_, totalTime_;

	void frameStatistics(long cameraTime, long processingTime, long totalTime);

	int width_;
	int height_;
	int fps_;
	int bytesPerSourcePixel_;
	int bytesPerDestPixel_;

	std::vector<FrameProcessor*> processorList;
	std::vector<FrameProcessor*>::iterator frame;

#ifndef WIN32
    pthread_t cameraThread;
#else
    HANDLE cameraThread;
#endif

	std::string app_name_;
	std::vector<std::string> help_text;
    enum DisplayMode { NO_DISPLAY, SOURCE_DISPLAY, DEST_DISPLAY };
};

#endif
