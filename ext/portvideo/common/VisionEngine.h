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

#ifndef VISIONENGINE_H
#define VISIONENGINE_H

#include <iostream>
#include <iomanip>
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

#include "Main.h"
#include "CameraTool.h"
#include "RingBuffer.h"
#include "UserInterface.h"

void pv_sleep(int ms=1);

class VisionEngine
{

public:
	VisionEngine(const char* name, application_settings *config);
	~VisionEngine() {
        if (camera_) delete camera_;
        if (interface_) {
            app_config_->display_mode=interface_->getDisplayMode();
            delete interface_;
        }
    };

	void start();
	void stop();
    void event(int key);
    void pause(bool pause);
	
	void setupCamera();
	void teardownCamera();
	void resetCamera(CameraConfig *cam_cfg = NULL);

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

    static long currentSeconds() {
        time_t currentTime;
        time(&currentTime);
        return (long) currentTime;
    }
    
    static unsigned long currentMicroSeconds() {
#ifdef WIN32
        FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		unsigned long long tt = ft.dwHighDateTime;
		tt <<=32;
		tt |= ft.dwLowDateTime;
		tt /=10;
		return (unsigned long)(tt - 11644473600000000ULL);
#else
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv,&tz);
        return ((tv.tv_sec*1000000)+(tv.tv_usec));
#endif
    }

protected:
	void initFrameProcessors();

	void allocateBuffers();
	void freeBuffers();

	void mainLoop();
    void startThread();
    void stopThread();
    

    
#ifndef NDEBUG
    void saveBuffer(unsigned char* buffer, int bytes);
    bool recording_;
#endif

    UserInterface *interface_;

private:
	long frames_;
  	long lastTime_;
	unsigned int cameraTime_, processingTime_, interfaceTime_, totalTime_;

	void frameStatistics(long cameraTime, long processingTime,long totalTime);

	int width_;
	int height_;
	int fps_;
	int format_;

    unsigned char* sourceBuffer_;
    unsigned char* destBuffer_;
    
	std::vector<FrameProcessor*> processorList;
	std::vector<FrameProcessor*>::iterator frame;

#ifndef WIN32
    pthread_t cameraThread;
#else
    HANDLE cameraThread;
#endif
    
    application_settings *app_config_;
    CameraConfig *camera_config_;

	std::string app_name_;
};

#endif
