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

#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <stdio.h>
#include <string>
#include <vector>
#include "UserInterface.h"
#include "CameraEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class FrameProcessor
{
public:

	FrameProcessor() {
		width     = 0;
		height    = 0;
		src_format  = 0;
		dest_format = 0;
		initialized = false;
		ui = NULL;
	};
	virtual ~FrameProcessor() {};

	virtual bool init(int w, int h, int sf, int df) {
		width  = w;
		height = h;
		src_format  = sf;
		dest_format = df;

		initialized = true;
		return true;
	};

    virtual void addUserInterface(UserInterface *uiface) { ui=uiface; };
    virtual void process(unsigned char *src, unsigned char *dest) = 0;
    virtual bool setFlag(unsigned char flag, bool value, bool lock) { return lock; };
    virtual bool toggleFlag(unsigned char flag, bool lock) { return lock; };
	std::vector<std::string> getOptions() { return help_text; }

protected:
	int width;
	int height;
	int src_format;
	int dest_format;

	bool initialized;
	UserInterface *ui;

	std::vector<std::string> help_text;
};

#endif
