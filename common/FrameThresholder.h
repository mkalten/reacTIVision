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

#ifndef FRAMETHRESHOLDER_H
#define FRAMETHRESHOLDER_H

#include "FrameProcessor.h"
#include "FontTool.h"
#include "SDLinterface.h"
#include "tiled_bernsen_threshold.h"
#include "threshold.h"

class FrameThresholder: public FrameProcessor
{
public:
	FrameThresholder(short g, int s, int t) {
		initialized = false;

		gradient = g;
 		if (gradient<0) gradient=0;
		if (gradient>128) gradient=128;
		setGradient = false;

        tile_size = s;
        if (tile_size<2) tile_size=2;
        setTilesize = false;
        
        thread_count = t;
        if (thread_count<1) thread_count = 1;
		else if (thread_count>16) thread_count = 16;
	};
	~FrameThresholder() {
		if (initialized) {

            for (int i=0;i<thread_count;i++) {
                terminate_tiled_bernsen_thresholder( thresholder[i] );
                delete thresholder[i];
            }

			delete tile_sizes;
            delete thresholder;
		}
	};

	void process(unsigned char *src, unsigned char *dest, SDL_Surface *display);
	bool init(int w ,int h, int sb, int db);
	void drawGUI(SDL_Surface *display);

	void setFlag(int flag, bool value);
	void toggleFlag(int flag);

	int getGradientGate() { return gradient; };
    int getTileSize() { return tile_size; };

private:
	TiledBernsenThresholder **thresholder;
	short gradient;
	bool setGradient;
	short tile_size;
    short *tile_sizes;
    short tile_count;
    short tile_index;
    bool setTilesize;
    int thread_count;
 };

#endif
