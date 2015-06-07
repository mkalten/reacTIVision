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

#include "CameraTool.h"
#include "UserInterface.h"
#include "SFont.h"
#include "Resources.h"

class SDLinterface: public UserInterface
{

public:
	SDLinterface(const char* name, bool fullscreen);
	~SDLinterface() {};

	unsigned char* openDisplay(VisionEngine *engine);
    void updateDisplay();
    void closeDisplay();
    
    void setHelpText(std::vector<std::string> hlp);
    void setBuffers(unsigned char *src, unsigned char *dest, int width, int height, bool color);
    void processEvents();
    void setDisplayMode(DisplayMode mode);
    
    void printMessage(std::string message);
    void displayMessage(const char *message);
    void displayControl(const char *title, int min, int max, int value);
    void displayError(const char* error);
    void drawMark(int xpos, int ypos, const char *mark, int state);
    
    void setVsync(bool vsync);

protected:
	
    bool setupWindow();
    char* checkRenderDriver();

    void allocateBuffers();
	void freeBuffers();

	void toggleFullScreen();
	void drawHelp();
    void showFrameRate();
	
	void selectCamera();

private:
    
    SDL_Renderer *renderer_;
    SDL_Window *window_;
    SDL_Surface *displayImage_;
    
    unsigned char* sourceBuffer_;
    unsigned char* destBuffer_;
    
    SDL_Texture *texture_;
    SDL_Texture *display_;
	
	SFont_Font *sfont_;
	int textHeight();
	int textWidth(const char *text);
	void drawText(int xpos, int ypos, const char* text);
	
    bool error_;
    bool pause_;
    bool help_;
	bool select_;
#ifndef NDEBUG
    bool recording_;
#endif
    bool fullscreen_;
	bool vsync_;
   
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
	
	std::vector<CameraConfig> dev_list;
	int selector_;
};

#endif
