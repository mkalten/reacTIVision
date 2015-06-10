/*  reacTIVision tangible interaction framework
    FrameThresholder.cpp
    Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

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
#ifndef WIN32
static void* threshold_thread_function( void* obj )
#else
static DWORD WINAPI threshold_thread_function( LPVOID obj )
#endif
{
    threshold_data *data = (threshold_data *)obj;
    tiled_bernsen_threshold( data->thresholder, data->dest, data->src,data->bytes, data->width, data->height, data->tile_size, data->gradient );

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
    
    help_text.push_back( "FrameThresholder:");
	help_text.push_back( "   g - set gradient gate & tile size");

	return true;
}

void FrameThresholder::process(unsigned char *src, unsigned char *dest) {


    //long start_time = VisionEngine::currentMicroSeconds();
    //tiled_bernsen_threshold( thresholder, dest, src, srcBytes, width, height, tile_size, gradient );
    
#ifdef WIN32
    HANDLE tthreads[16];
#else
    pthread_t tthreads[16];
#endif
    threshold_data tdata[16];

    for (int i=0;i<thread_count;i++) {
        int part_height = height/thread_count;
        unsigned char *sp = src+i*part_height*width;
        unsigned char *dp = dest+i*part_height*width;

        //tdata[i] = { .thresholder=thresholder[i], .src=sp, .dest=dp, .width=width, .height=part_height, .bytes=srcBytes, .tile_size=tile_size, .gradient=gradient };

		tdata[i].thresholder=thresholder[i];
		tdata[i].src=sp;
		tdata[i].dest=dp;
		tdata[i].width=width;
		tdata[i].height=part_height;
		tdata[i].bytes=src_format;
		tdata[i].tile_size=tile_size;
		tdata[i].gradient=gradient;

#ifdef WIN32
        DWORD threadId;
        tthreads[i] = CreateThread( 0, 0, threshold_thread_function, &tdata[i], 0, &threadId );
#else
        pthread_create(&tthreads[i] , NULL, threshold_thread_function, &tdata[i]);
#endif
    }

    if (setGradient || setTilesize) displayControl();

    for (int i=0;i<thread_count;i++) {
#ifdef WIN32
        WaitForSingleObject(tthreads[i],INFINITE);
		CloseHandle(tthreads[i]);
#else
        pthread_join(tthreads[i],NULL);
#endif

    }

    //long threshold_time = VisionEngine::currentMicroSeconds() - start_time;
    //float latency = threshold_time/1000.0f;
    //std::cout << "threshold latency: " << latency << "ms" << std::endl;
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
  
    ui->displayControl(displayText, 0, maxValue, settingValue);
}

