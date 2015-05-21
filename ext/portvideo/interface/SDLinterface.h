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


#ifndef SDLINTERFACE_H
#define SDLINTERFACE_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#endif

#include <sys/stat.h>

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#else
#include <SDL.h>
#include <SDL_thread.h>
#endif
#include <time.h>

#define WIDTH 640
#define HEIGHT 480

#include "UserInterface.h"
#include "FontTool.h"
#include "Resources.h"

class SDLinterface: public UserInterface
{

public:
	SDLinterface(const char* name, bool fullscreen);
	~SDLinterface() {};

	unsigned char* openDisplay(VisionEngine *engine);
    void updateDisplay();
    void closeDisplay();
    
    void setBuffers(unsigned char *src, unsigned char *dest, int width, int height, bool color);
    void processEvents();
    void setDisplayMode(DisplayMode mode);
    
    void printMessage(std::string message);
    void displayMessage(const char *message);
    void displayControl(const char *title, int min, int max, int value);
    void displayError(const char* error);
    void drawMark(int xpos, int ypos, const char *mark, int state);

protected:
	
    bool setupWindow();
    char* checkRenderDriver();

    void allocateBuffers();
	void freeBuffers();

	void toggleFullScreen();
	void drawHelp(unsigned char* buffer);
    void showFrameRate();

private:
    
    long currentTime() {
        time_t currentTime;
        time(&currentTime);
        return (long) currentTime;
    }
    
    SDL_Renderer *renderer_;
    SDL_Window *window_;
    SDL_Surface *displayImage_;
    
    unsigned char* sourceBuffer_;
    unsigned char* destBuffer_;
    
    SDL_Texture *texture_;
    SDL_Texture *display_;
    
    bool error_;
    bool pause_;
    bool help_;
#ifndef NDEBUG
    bool recording_;
#endif
    bool fullscreen_;
   
	long frames_;
  	long lastTime_;
	unsigned int cameraTime_, processingTime_, totalTime_;
    unsigned int current_fps_;
    
	int width_;
	int height_;
	int bytesPerSourcePixel_;
	int bytesPerDestPixel_;

	std::string app_name_;
	std::vector<std::string> help_text;
};

#endif
