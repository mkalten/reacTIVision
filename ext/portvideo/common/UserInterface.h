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

#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <stdio.h>
#include <string>
#include <vector>

#define KEY_A 4
#define KEY_B 5
#define KEY_C 6
#define KEY_D 7
#define KEY_E 8
#define KEY_F 9
#define KEY_G 10
#define KEY_H 11
#define KEY_I 12
#define KEY_J 13
#define KEY_K 14
#define KEY_L 15
#define KEY_M 16
#define KEY_N 17
#define KEY_O 18
#define KEY_P 19
#define KEY_Q 20
#define KEY_R 21
#define KEY_S 22
#define KEY_T 23
#define KEY_U 24
#define KEY_V 25
#define KEY_W 26
#define KEY_X 27
#define KEY_Y 28
#define KEY_Z 29

#define KEY_SPACE 44
#define KEY_RIGHT 79
#define KEY_LEFT 80
#define KEY_DOWN 81
#define KEY_UP 82

#define WIDTH 640
#define HEIGHT 480

enum DisplayMode { NO_DISPLAY, SOURCE_DISPLAY, DEST_DISPLAY };

class VisionEngine;
class UserInterface
{
public:
	UserInterface() {
		width_ = WIDTH;
		height_ = HEIGHT;
		format_ = 0;
		verbose_ = false;
	};
	virtual ~UserInterface() {};
    
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; };
    DisplayMode getDisplayMode() { return displayMode_; };
    
    virtual bool openDisplay(VisionEngine *engine) = 0;
    virtual void updateDisplay() = 0;
    virtual void closeDisplay() = 0;
    
    virtual void setHelpText(std::vector<std::string> hlp) = 0;
    virtual void setBuffers(unsigned char *src, unsigned char *dest, int width, int height, int format) = 0;
    virtual void processEvents() = 0;
    void setVerbose(bool verbose) { verbose_ = verbose; };
    
	virtual void printMessage(std::string message) = 0;
	virtual void displayMessage(const char *message) = 0;
    virtual void displayControl(const char *title, int min, int max, int value) = 0;
    virtual void displayError(const char *message) = 0;
    virtual void drawText(int xpos, int ypos, const char *text) = 0;
	
	virtual void setColor(unsigned char r, unsigned char g, unsigned char b) = 0;
	virtual void drawPoint(int x,int y) = 0;
	virtual void drawLine(int x1,int y1, int x2, int y2) = 0;
	virtual void drawRect(int x,int y, int w, int h) = 0;
	virtual void drawRect(int x,int y, int w, int h, float r) = 0;
	virtual void fillRect(int x,int y, int w, int h) = 0;
	virtual void drawEllipse(int x,int y, int w, int h) = 0;
	virtual void drawEllipse(int x,int y, int w, int h, float r) = 0;
	virtual void fillEllipse(int x,int y, int w, int h) = 0;
    
protected:
	bool verbose_;
    DisplayMode displayMode_;
    VisionEngine *engine_;
	
	unsigned char* sourceBuffer_;
	unsigned char* destBuffer_;
	
	int width_;
	int height_;
	int format_;

};

#endif
