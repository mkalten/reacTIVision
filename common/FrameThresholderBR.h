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

#ifndef FRAMETHRESHOLDER_BR_H
#define FRAMETHRESHOLDER_BR_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#include "FrameProcessor.h"
#include "../ext/libfidtrack/bradley_roth_threshold.h"

#ifdef WIN32
void usleep(long value);
#endif

typedef struct threshold_br_data {
	bool process;
#ifdef WIN32
	HANDLE ghWriteEvent;
#else
	pthread_cond_t cond;
	pthread_mutex_t mutex;
#endif
	BradleyRothThresholder *thresholder;
	unsigned char *src;         /* padded source pointer (halo rows prepended) */
	unsigned char *dest;
	int width;
	int padded_height;          /* strip_height + halo rows above + below */
	int strip_height;           /* actual output rows for this thread */
	int src_row_offset;         /* halo rows prepended in src */
	int bytes;
	int window_size;
	float bias;
	int min_contrast;
	unsigned char *map;         /* equalizer map, points into unpadded region */
	int map_width;              /* width for equalizer row stride */
	int average;
	bool done;
	int id;
} threshold_br_data;

class FrameThresholderBR: public FrameProcessor
{
public:
	FrameThresholderBR(int window, float bias_val, int mc, int t) {
		initialized = false;

		window_size = window;
		if (window_size < 1) window_size = 1;
		setThreshold      = false;
		threshold_setting = 0;

		bias = bias_val;
		min_contrast = mc;
		if (min_contrast < 0) min_contrast = 0;
		else if (min_contrast > 255) min_contrast = 255;

		thread_count = t;
		if (thread_count < 1) thread_count = 1;
		else if (thread_count > 16) thread_count = 16;

		equalize  = false;
		calibrate = false;

	};

	~FrameThresholderBR() {
		if (initialized) {
			for (int i = 0; i < thread_count; i++) {
#ifdef WIN32
				tdata[i].done = true;
				SetEvent(tdata[i].ghWriteEvent);
#else
				pthread_mutex_lock(&tdata[i].mutex);
				tdata[i].done = true;
				pthread_cond_signal(&tdata[i].cond);
				pthread_mutex_unlock(&tdata[i].mutex);
#endif
				while (tdata[i].process) usleep(10);
				terminate_bradley_roth_thresholder(thresholder[i]);
				delete thresholder[i];
			}
			delete[] thresholder;
			delete[] pointmap;
		}
	};

	void process(unsigned char *src, unsigned char *dest);
	bool init(int w, int h, int sb, int db);
	void displayControl();

	bool setFlag(unsigned char flag, bool value, bool lock);
	bool toggleFlag(unsigned char flag, bool lock);

	float getBias() { return bias; };
	int getWindowSize() { return window_size; };
	int getMinContrast() { return min_contrast; };
	bool getEqualizerState() { return equalize; };

private:
	BradleyRothThresholder **thresholder;
	int padded_strip_height;    /* strip_height + 2*window_size, clamped */
	int   window_size;
	bool  setThreshold;         /* threshold config mode active */
	int   threshold_setting;    /* 0=window_size, 1=bias */
	float bias;
	int   min_contrast;
	int   thread_count;

	unsigned char *pointmap;
	int   average;
	bool  equalize;
	bool  calibrate;

#ifdef WIN32
	HANDLE tthreads[16];
#else
	pthread_t tthreads[16];
#endif
	threshold_br_data tdata[16];
};

#endif
