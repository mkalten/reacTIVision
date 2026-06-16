/*  reacTIVision tangible interaction framework
	Copyright (C) 2005-2017 Martin Kaltenbrunner <martin@tuio.org>

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

#include "FrameThresholderBR.h"

#ifdef WIN32
static DWORD WINAPI threshold_br_thread_function( LPVOID obj )
#else
static void* threshold_br_thread_function( void *obj )
#endif
{
	threshold_br_data *data = (threshold_br_data *)obj;

	while (!data->done) {

#ifdef WIN32
		DWORD dwWaitResult = WaitForSingleObject(data->ghWriteEvent, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0) {
			data->process = false;
			continue;
		}
#else
		pthread_mutex_lock(&data->mutex);
		while (!data->process && !data->done)
			pthread_cond_wait(&data->cond, &data->mutex);
		pthread_mutex_unlock(&data->mutex);
#endif
		if (data->done) return(0);

		/* equalizer — only touches the actual strip rows, not the halo */
		if (data->average >= 0) {
			int e = 0;
			unsigned char *strip = data->src + data->src_row_offset * data->width;
			unsigned char *map   = data->map;
			for (int i = data->width * data->strip_height; i > 0; i--) {
				e = *strip - (*map++ - data->average);
				if (e & (~255)) { e < 0 ? *strip++ = 0 : *strip++ = 255; }
				else *strip++ = (unsigned char)e;
			}
		}

		/* thresholder — src points to padded region */
		bradley_roth_threshold( data->thresholder, data->dest, data->src,
		                        data->width, data->padded_height, data->strip_height,
		                        data->src_row_offset,
		                        data->window_size, data->bias, data->min_contrast );

		data->process = false;
	}

	return(0);
}


bool FrameThresholderBR::init(int w, int h, int sb, int db) {

	if (initialized) {
		for (int i = 0; i < thread_count; i++) {
			tdata[i].done = true;
#ifdef WIN32
			SetEvent(tdata[i].ghWriteEvent);
#else
			pthread_cond_signal(&tdata[i].cond);
#endif
			while (tdata[i].process) usleep(10);
			terminate_bradley_roth_thresholder(thresholder[i]);
			delete thresholder[i];
		}
		delete[] thresholder;
		delete[] pointmap;
	}

	FrameProcessor::init(w, h, sb, db);

	int th = h / thread_count;
	while (th % 2 != 0) {
		thread_count--;
		if (thread_count == 1) { th = h; break; }
		th = h / thread_count;
	}

	/* compute max window size from geometry, then clamp config value */
	max_window = th / 2;
	if (max_window < 1) max_window = 1;
	if (window_size > max_window) window_size = max_window;

	/* padded height covers full max halo */
	padded_strip_height = th + 2 * max_window;
	if (padded_strip_height > h) padded_strip_height = h;

	thresholder = new BradleyRothThresholder*[thread_count];
	for (int i = 0; i < thread_count; i++) {
		thresholder[i] = new BradleyRothThresholder();
		initialize_bradley_roth_thresholder(thresholder[i], w, padded_strip_height);
	}

	average  = 0;
	int size = width * height;
	pointmap = new unsigned char[size];
	for (int i = 0; i < size; i++) pointmap[i] = 0;

	for (int i = 0; i < thread_count; i++) {
		tdata[i].process = false;
		tdata[i].done    = false;

		tdata[i].thresholder = thresholder[i];
		tdata[i].bytes       = src_format;
		tdata[i].id          = i;

#ifdef WIN32
		DWORD threadId;
		tdata[i].ghWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		tthreads[i] = CreateThread(0, 0, threshold_br_thread_function, &tdata[i], 0, &threadId);
#else
		tdata[i].cond  = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
		tdata[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		pthread_create(&tthreads[i], NULL, threshold_br_thread_function, &tdata[i]);
		pthread_detach(tthreads[i]);
#endif
	}

	help_text.push_back( "FrameThresholderBR (Bradley-Roth):");
	help_text.push_back( "   g - set window size & bias");
	help_text.push_back( "   e - activate frame equalization");
	help_text.push_back( "   SPACE - reset frame equalization");

	return true;
}


void FrameThresholderBR::process(unsigned char *src, unsigned char *dest) {

	if (calibrate) {
		unsigned int sum = 0;
		for (int x = width/2-5; x < width/2+5; x++)
			for (int y = height/2-5; y < height/2+5; y++)
				sum += src[y * width + x];

		average = (unsigned char)(sum / 100);
		memcpy(pointmap, src, width * height);
		calibrate = false;
		equalize  = true;
	}

	for (int i = 0; i < thread_count; i++) {
		int strip_height = height / thread_count;
		int strip_start  = i * strip_height;

		int top_halo    = (strip_start >= window_size) ? window_size : strip_start;
		int padded_top  = strip_start - top_halo;
		int strip_end   = strip_start + strip_height;
		int bot_halo    = (strip_end + window_size <= height) ? window_size : height - strip_end;
		int ph          = top_halo + strip_height + bot_halo;
		if (ph > padded_strip_height) ph = padded_strip_height;

		tdata[i].src            = src  + padded_top * width * src_format;
		tdata[i].dest           = dest + strip_start * width;
		tdata[i].width          = width;
		tdata[i].strip_height   = strip_height;
		tdata[i].padded_height  = ph;
		tdata[i].src_row_offset = top_halo;

		tdata[i].window_size   = window_size;
		tdata[i].bias          = bias;
		tdata[i].min_contrast  = min_contrast;

		if (equalize) {
			tdata[i].average   = average;
			tdata[i].map       = pointmap + strip_start * width;
			tdata[i].map_width = width;
		} else {
			tdata[i].average = -1;
			tdata[i].map     = NULL;
		}

#ifdef WIN32
		tdata[i].process = true;
		SetEvent(tdata[i].ghWriteEvent);
#else
		pthread_mutex_lock(&tdata[i].mutex);
		tdata[i].process = true;
		pthread_cond_signal(&tdata[i].cond);
		pthread_mutex_unlock(&tdata[i].mutex);
#endif
	}

	for (int i = 0; i < thread_count; i++)
		while (tdata[i].process) usleep(10);

	if (setThreshold) displayControl();
}


bool FrameThresholderBR::setFlag(unsigned char flag, bool value, bool lock) {
	if (flag == KEY_G) setThreshold = value;
	return false;
}


bool FrameThresholderBR::toggleFlag(unsigned char flag, bool lock) {

	if (flag == KEY_G) {
		if (setThreshold) {
			setThreshold = false;
			return false;
		} else if (!lock) {
			setThreshold = true;
			threshold_setting = 0;
			return true;
		} else return false;
	} else if (setThreshold) {
		switch (flag) {
		case KEY_LEFT:
			if (threshold_setting == 0) {
				bias -= 0.01f;
				if (bias < 0.0f) bias = 0.0f;
			} else if (threshold_setting == 1) {
				window_size--;
				if (window_size < 1) window_size = 1;
			} else {
				min_contrast--;
				if (min_contrast < 0) min_contrast = 0;
			}
			break;
		case KEY_RIGHT:
			if (threshold_setting == 0) {
				bias += 0.01f;
				if (bias > 0.5f) bias = 0.5f;
			} else if (threshold_setting == 1) {
				window_size++;
				if (window_size > max_window) window_size = max_window;
			} else {
				min_contrast++;
				if (min_contrast > 255) min_contrast = 255;
			}
			break;
		case KEY_UP:
			threshold_setting--;
			if (threshold_setting < 0) threshold_setting = 2;
			break;
		case KEY_DOWN:
			threshold_setting++;
			if (threshold_setting > 2) threshold_setting = 0;
			break;
		}
	} else if (flag == KEY_E) {
		equalize  = !equalize;
		calibrate = false;
	} else if (flag == KEY_SPACE) {
		equalize  = false;
		calibrate = true;
	}

	return lock;
}


void FrameThresholderBR::displayControl() {

	char displayText[64];
	ui->drawText(17, 14, "G          - exit threshold configuration");

	if (threshold_setting == 0) {
		snprintf(displayText, 64, "gradient %.2f", bias);
		ui->displayControl(displayText, 0, 100, (int)(bias * 200.0f));
	} else if (threshold_setting == 1) {
		snprintf(displayText, 64, "window size %d", window_size);
		ui->displayControl(displayText, 1, max_window, window_size);
	} else {
		snprintf(displayText, 64, "min contrast %d", min_contrast);
		ui->displayControl(displayText, 0, 255, min_contrast);
	}
}
