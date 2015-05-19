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

#ifndef FONTTOOL_H
#define FONTTOOL_H

#ifdef __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "Resources.h"
#include "SFont.h"

class FontTool {

public:	
	static void init(SDL_Surface *disp);
	static void close();
	
	static void drawText(int xpos, int ypos, const char* text);
	static int getFontHeight();
	static int getTextWidth(const char *text);
private:
	static SFont_Font *sfont;
    static SDL_Surface *display;
	
};

#endif
