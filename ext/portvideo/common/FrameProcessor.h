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

#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <stdio.h>
#include <string>
#include <vector>
#include "UserInterface.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

class FrameProcessor
{
public:

	FrameProcessor() {
		width     = 0;
		height    = 0;
		srcBytes  = 0;
		destBytes = 0;
		srcSize   = 0;
		destSize  = 0;
		dispSize  = 0;
		initialized = false;
	};
	virtual ~FrameProcessor() {};

	virtual bool init(int w, int h, int sb, int db) {
		width  = w;
		height = h;
		srcBytes  = sb;
		destBytes = db;

		srcSize   = w*h*sb;
		destSize  = w*h*db;
		dispSize  = w*h*4;

		initialized = true;
		return true;
	};

    virtual void addUserInterface(UserInterface *uiface) { interface_=uiface; };
    virtual void process(unsigned char *src, unsigned char *dest, unsigned char *display) = 0;
    virtual bool setFlag(unsigned char flag, bool value, bool lock) { return lock; };
    virtual bool toggleFlag(unsigned char flag, bool lock) { return lock; };
	std::vector<std::string> getOptions() { return help_text; }

protected:
	int width;
	int height;
	int srcBytes;
	int destBytes;
	int srcSize;
	int destSize;
	int dispSize;

	bool initialized;
	UserInterface *interface_;

	std::vector<std::string> help_text;
};

#endif
