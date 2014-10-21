/*  reacTIVision tangible interaction framework
    FrameThresholder.h
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

#ifndef FRAMEEQUALIZER_H
#define FRAMEEQUALIZER_H

#include "FrameProcessor.h"
#include "SDLinterface.h"


class FrameEqualizer: public FrameProcessor
{
public:	
	FrameEqualizer() {
		initialized = false;
		equalize = false;
		calibrate = false;
	};
	~FrameEqualizer() {
		if (initialized) {
				delete[] pointmap;
		}
	};
	
	void process(unsigned char *src, unsigned char *dest, SDL_Surface *display);
	bool init(int w ,int h, int sb, int db);
	void toggleFlag(int flag);
	bool getState() { return equalize; };
	
private:
    unsigned char *pointmap;
	bool equalize;
	bool calibrate;
};

#endif
