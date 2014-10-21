/*  reacTIVision tangible interaction framework
    Resources.h
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

#ifndef _images_H_
#define _images_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <SDL.h>
extern unsigned char icon[3126];
extern unsigned char mask[1024];
extern unsigned char camera[3910];
extern Uint8 *getMask();
extern SDL_Surface *getIcon();
extern SDL_Surface *getCamera();
extern SDL_Rect camera_rect;
#ifdef __cplusplus
}
#endif
#endif
