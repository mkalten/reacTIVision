/*  SFont: a simple font-library that uses special .pngs as fonts
    Copyright (C) 2003 Karl Bartel

    License: GPL or LGPL (at your choice)
    WWW: http://www.linux-games.com/sfont/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Karl Bartel
    Cecilienstr. 14
    12307 Berlin
    GERMANY
    karlb@gmx.net
*/

/************************************************************************ 
*    SFONT - SDL Font Library by Karl Bartel <karlb@gmx.net>            *
*                                                                       *
*  All functions are explained below. For further information, take a   *
*  look at the example files, the links at the SFont web site, or       *
*  contact me, if you problem isn' addressed anywhere.                  *
*                                                                       *
************************************************************************/
#ifndef SFONT_H
#define SFONT_H

#include <SDL.h>

#ifdef __cplusplus 
extern "C" {
#endif

// Delcare one variable of this type for each font you are using.
// To load the fonts, load the font image into YourFont->Surface
// and call InitFont( YourFont );
typedef struct {
	SDL_Surface *Surface;	
	int CharPos[512];
	int MaxPos;
} SFont_Font;

// 10.01.2006 Martin Kaltenbrunner <martin@tuio.org>
// added binary default font and methods to load the default font
SDL_Surface* SFont_FontSurface ();
SFont_Font* SFont_InitDefaultFont ();

// Initializes the font
// Font: this contains the surface with the font.
//       The Surface must be loaded before calling this function

SFont_Font* SFont_InitFont (SDL_Surface *Font);

// Frees the font
// Font: The font to free
//       The font must be loaded before using this function.
void SFont_FreeFont(SFont_Font* Font);

// Blits a string to a surface
// Destination: the suface you want to blit to
// text: a string containing the text you want to blit.
void SFont_Write(SDL_Surface *Surface, const SFont_Font *Font, int x, int y,
				 const char *text);

// Returns the width of "text" in pixels
int SFont_TextWidth(const SFont_Font* Font, const char *text);
// Returns the height of "text" in pixels (which is always equal to Font->Surface->h)
int SFont_TextHeight(const SFont_Font* Font);

// Blits a string to Surface with centered x position
void SFont_WriteCenter(SDL_Surface *Surface, const SFont_Font* Font, int y,
					   const char *text);

#ifdef __cplusplus
}
#endif

#endif /* SFONT_H */
