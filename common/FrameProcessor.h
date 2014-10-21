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

#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <SDL.h>
#include <stdio.h>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class MessageListener
{
public:
	MessageListener() { verbose_=false; };	
	virtual ~MessageListener() {};
	enum DisplayMode { NO_DISPLAY, SOURCE_DISPLAY, DEST_DISPLAY };	
	virtual void setDisplayMode(DisplayMode message) = 0;
	virtual DisplayMode getDisplayMode() = 0;
	virtual void setMessage(std::string message) = 0;
	virtual void displayMessage(const char *message) = 0;
protected:
	bool verbose_;
};

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

	virtual void process(unsigned char *src, unsigned char *dest, SDL_Surface *display) = 0;

	virtual void addMessageListener(MessageListener *listener) { msg_listener=listener; };
	virtual void removeMessageListener(MessageListener *listener) { msg_listener=NULL; };
	virtual void finish() {};
	virtual void setFlag(int flag, bool value) {};
	virtual void toggleFlag(int flag) {};
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
	MessageListener *msg_listener;

	std::vector<std::string> help_text;
};

#endif
