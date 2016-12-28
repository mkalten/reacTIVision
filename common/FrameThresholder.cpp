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

#include "FrameThresholder.h"
#include "VisionEngine.h"

// the thread function
#ifdef WIN32
void usleep(long value) {
	struct timeval tv;
	tv.tv_sec = value / 1000000;
	tv.tv_usec = value % 1000000;
	select(0, NULL, NULL, NULL, &tv);
}
static DWORD WINAPI threshold_thread_function( LPVOID obj )
#else
static void* threshold_thread_function( void *obj )
#endif
{
	threshold_data *data = (threshold_data *)obj;

	while (!data->done) {

#ifdef WIN32
		DWORD dwWaitResult  = WaitForSingleObject(data->ghWriteEvent, INFINITE);
		if ( dwWaitResult != WAIT_OBJECT_0) {
			data->process = false;
			continue;
		}
#else
		pthread_cond_wait(&data->cond, &data->mutex);
#endif
		if (data->done) return(0);

		// equalizer
		if (data->average>=0) {
			int e = 0;
			unsigned char* src = data->src;
			unsigned char *map = data->map;
			for (int i=data->width*data->height;i>0;i--) {
				e = *src - (*map++ - data->average);
				if (e & (~255)) { e < 0 ? *src++ = 0 : *src++ = 255; }
				else *src++ = (unsigned char)e;
			}
		}

		// thresholder
		tiled_bernsen_threshold( data->thresholder, data->dest, data->src,data->bytes, data->width, data->height, data->tile_size, data->gradient );

		data->process = false;
	}

	return(0);
}

int getDividers(short number, short *dividers) {

	short count=0;

	for (int i=1;i<number;i++) {

		if ((number%i)==0) {
			if (i > number/i) break;
			dividers[count] = i;
			dividers[number-count] = number/i;
			count++;
		}

	}

	for (int i=0;i<count;i++ ) dividers[count+i] = dividers[(number-count+1)+i];
	count = count*2;

	/*printf("dividers %d: ",number);
	for (int i=0;i<count;i++ ) printf("%d ",dividers[i]);
	printf("\n");*/

	return count;

}

bool FrameThresholder::init(int w, int h, int sb, int db) {


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

	FrameProcessor::init(w,h,sb,db);

	short tw = w;
	short th = h/thread_count;

	while (th%2!=0) {
		thread_count--;
		if (thread_count==1) {
			th = h;
			break;
		}
		th = h/thread_count;
	}

	short tw_div[8192];
	short dw_count = getDividers(tw, tw_div);

	short th_div[8182];
	short dh_count=getDividers(th, th_div);

	tile_sizes = new short[dw_count];

	tile_index=-1;
	tile_count=0;
	for (int i=1;i<dw_count;i++) {
		for (int j=1;j<dh_count;j++) {
			if (tw_div[i]==th_div[j]) {
				tile_sizes[tile_count] = tw_div[i];
				if (tile_size==tile_sizes[tile_count]) tile_index=tile_count;
				tile_count++;
			}
		}
	}

	if (tile_index<0) {
		for (int i=1;i<tile_count;i++) {
			if ((tile_sizes[i-1] < tile_size) && (tile_size < tile_sizes[i])) {
				tile_index=i-1;
				//tile_size = tile_sizes[tile_index];
				break;
			}
		}
	}

	if (tile_index<0) {
		tile_index=tile_count-1;
		//tile_size = tile_sizes[tile_index];
	}

	/*printf("%d common dividers %d %d: ",tile_count,tw,th);
	for (int i=0;i<tile_count;i++) printf("%d ",tile_sizes[i]);
	printf("\n");*/

	thresholder = new TiledBernsenThresholder*[thread_count];
	for(int i=0;i<thread_count;i++) {
		thresholder[i] = new TiledBernsenThresholder();
		initialize_tiled_bernsen_thresholder(thresholder[i], tw, th, 2 );
	}

	average = 0;
	int size = width*height;
	pointmap = new unsigned char[size];
	for (int i=0;i<size;i++) pointmap[i] = 0;

	for (int i=0;i<thread_count;i++) {

		tdata[i].process=false;
		tdata[i].done=false;

		tdata[i].thresholder=thresholder[i];
		tdata[i].bytes=src_format;

		tdata[i].id=i;

#ifdef WIN32
		DWORD threadId;
		tdata[i].ghWriteEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("ProcessEvent"+i));
		tthreads[i] = CreateThread( 0, 0, threshold_thread_function, &tdata[i], 0, &threadId );
#else
		tdata[i].cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
		tdata[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		pthread_create(&tthreads[i] , NULL, threshold_thread_function, &tdata[i]);
		pthread_detach(tthreads[i]);
#endif
	}

	help_text.push_back( "FrameThresholder:");
	help_text.push_back( "   g - set gradient gate & tile size");
	help_text.push_back( "   e - activate frame equalization");
	help_text.push_back( "   SPACE - reset frame equalization");

	return true;
}

void FrameThresholder::process(unsigned char *src, unsigned char *dest) {

	//unsigned long start_time = VisionEngine::currentMicroSeconds();

	if (calibrate) {

		unsigned int sum = 0;
		for (int x=width/2-5;x<width/2+5;x++) {
			for (int y=height/2-5;y<height/2+5;y++) {
				sum+=src[y*width+x];
			}
		}

		average = (unsigned char)(sum/100);
		memcpy(pointmap,src,width*height);

		calibrate = false;
		equalize = true;
	}

	for (int i=0;i<thread_count;i++) {

		int part_height = height/thread_count;
		int offset = i*part_height*width;

		tdata[i].src=src+offset;
		tdata[i].dest=dest+offset;

		tdata[i].width=width;
		tdata[i].height=part_height;

		tdata[i].tile_size=tile_size;
		tdata[i].gradient=gradient;

		if (equalize) {
			tdata[i].average = average;
			tdata[i].map = pointmap+offset;

		} else {
			tdata[i].average = -1;
			tdata[i].map = NULL;
		}

		tdata[i].process = true;
#ifdef WIN32
		SetEvent(tdata[i].ghWriteEvent);
#else
		pthread_cond_signal(&tdata[i].cond);
#endif
	}

	for (int i=0;i<thread_count;i++) {
		while(tdata[i].process) usleep(10);
	}

	if (setGradient || setTilesize) displayControl();

	//float frm_latency = (VisionEngine::currentMicroSeconds() - start_time)/1000.0f;
	//if (frm_latency<min_latency) min_latency = frm_latency;
	//if (frm_latency>max_latency) max_latency = frm_latency;
	//std::cout << "threshold latency: " << min_latency << " " << frm_latency << " " << max_latency << std::endl;
}

bool FrameThresholder::setFlag(unsigned char flag, bool value, bool lock) {
	if (flag==KEY_G) setGradient=value;
	return false;
}

bool FrameThresholder::toggleFlag(unsigned char flag, bool lock) {

	if (flag==KEY_G) {
		if ((setGradient) || (setTilesize)) {
			setGradient=false;
			setTilesize=false;
			return false;
		} else if (!lock) {
			setGradient=true;
			setTilesize=false;
			return true;
		} else return false;
	} else if ((setGradient) || (setTilesize)) {
		switch(flag) {
		case KEY_LEFT:
			if (setGradient) {
				gradient--;
				if (gradient<0) gradient=0;
			} else if (setTilesize) {
				tile_index--;
				if (tile_index<0) tile_index=0;
				tile_size = tile_sizes[tile_index];
			}
			break;
		case KEY_RIGHT:
			if (setGradient) {
				gradient++;
				if (gradient>64) gradient=64;
			} else if (setTilesize) {
				tile_index++;
				if (tile_index==tile_count) tile_index=tile_count-1;
				tile_size = tile_sizes[tile_index];
			}
			break;
		case KEY_UP:
		case KEY_DOWN:
			if (setGradient) {
				setGradient=false;
				setTilesize=true;
			} else {
				setGradient=true;
				setTilesize=false;
			}
			break;

		}
	} else if (flag==KEY_E) {
		equalize=!equalize;
		calibrate=false;
	} else if (flag==KEY_SPACE) {
		equalize=false;
		calibrate=true;
	}

	return lock;
}

void FrameThresholder::displayControl() {

	char displayText[64];
	int settingValue = 0;
	int maxValue = 0;

	if (setGradient) {

		settingValue = gradient;
		maxValue = 64;
		sprintf(displayText,"gradient gate %d",settingValue);
	} else if (setTilesize) {

		settingValue = tile_size;
		maxValue = tile_sizes[tile_count-1];
		if (tile_size>maxValue) maxValue=tile_size;
		sprintf(displayText,"tile size %d",settingValue);
	}

	ui->drawText(17,14,"G          - exit threshold configuration");
	ui->displayControl(displayText, 0, maxValue, settingValue);
}

