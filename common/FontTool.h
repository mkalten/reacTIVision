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

#ifndef FONTTOOL_H
#define FONTTOOL_H

#include <SDL.h>
#include "Resources.h"
#include "SFont.h"

class FontTool {

public:	
	static void init();
	static void close();
	
	static void drawText(int xpos, int ypos, const char* text, SDL_Surface *display);
	static int getFontHeight();
	static int getTextWidth(const char *text);
private:
	static SFont_Font *sfont;
	
};

#endif
