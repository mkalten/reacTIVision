/*  reacTIVision tangible interaction framework
	Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
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

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#include "FrameProcessor.h"
#include "tiled_bernsen_threshold.h"

#ifdef WIN32
void usleep(long value);
#endif

typedef struct threshold_data {
	bool process;
#ifdef WIN32
	HANDLE ghWriteEvent;
#else
	pthread_cond_t cond;
	pthread_mutex_t mutex;
#endif
	TiledBernsenThresholder *thresholder;
	unsigned char *src;
	unsigned char *dest;
	int width,height;
	int bytes;
	int tile_size;
	int gradient;
	unsigned char *map;
	int average;
	bool done;
	int id;
} threshold_data;

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
		
		equalize = false;
		calibrate = false;
		
		min_latency = 1000.0f;
		max_latency = 0.0f;
	};
	~FrameThresholder() {
		if (initialized) {
			
			for (int i=0;i<thread_count;i++) {
				tdata[i].done = true;
#ifdef WIN32
				SetEvent(tdata[i].ghWriteEvent);
#else
				pthread_cond_signal(&tdata[i].cond);
#endif
				while(tdata[i].process) usleep(10);
				terminate_tiled_bernsen_thresholder( thresholder[i] );
				delete thresholder[i];
			}

			delete[] tile_sizes;
			delete[] thresholder;
			delete[] pointmap;
		}
	};

    void process(unsigned char *src, unsigned char *dest);
	bool init(int w ,int h, int sb, int db);
	void displayControl();
	
	bool setFlag(unsigned char flag, bool value, bool lock);
	bool toggleFlag(unsigned char flag, bool lock);
	
	int getGradientGate() { return gradient; };
	int getTileSize() { return tile_size; };
	bool getEqualizerState() { return equalize; };
	
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

	unsigned char *pointmap;
	int average;
	bool equalize;
	bool calibrate;
	
	float min_latency, max_latency;
	
#ifdef WIN32
	HANDLE tthreads[16];
#else
	pthread_t tthreads[16];
#endif
	threshold_data tdata[16];
};

#endif
