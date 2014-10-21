/*  reacTIVision tangible interaction framework
    FrameThresholder.cpp
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

#include "FrameThresholder.h"
#include <SDL.h>
#include <SDL_thread.h>

typedef struct threshold_data{
    TiledBernsenThresholder *thresholder;
    unsigned char *src;
    unsigned char *dest;
    int width,height;
    int bytes;
    int tile_size;
    int gradient;
} threshold_data;

// the thread function which contantly retrieves the latest frame
int threshold_thread_function(void *obj) {

    threshold_data *data = (threshold_data *)obj;
    tiled_bernsen_threshold( data->thresholder, data->dest, data->src,data->bytes, data->width, data->height, data->tile_size, data->gradient );

    return(1);
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

void FrameThresholder::process(unsigned char *src, unsigned char *dest, SDL_Surface *display) {


    //long start_time = SDLinterface::currentTime();
    //tiled_bernsen_threshold( thresholder, dest, src, srcBytes, width, height, tile_size, gradient );
    SDL_Thread *tthreads[16];
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
		tdata[i].bytes=srcBytes;
		tdata[i].tile_size=tile_size;
		tdata[i].gradient=gradient;
    
		tthreads[i] = SDL_CreateThread(threshold_thread_function,NULL,&tdata[i]);
    }

    if (setGradient || setTilesize) drawGUI(display);

    for (int i=0;i<thread_count;i++) {
        SDL_WaitThread(tthreads[i],NULL);
    }

    /*long threshold_time = SDLinterface::currentTime() - start_time;
    float latency = (threshold_time/100.0f)/10.0f;
    std::cout << "threshold latency: " << latency << "ms" << std::endl;*/
}

void FrameThresholder::setFlag(int flag, bool value) {
	if (flag=='g') setGradient=value;
}

void FrameThresholder::toggleFlag(int flag) {

	if (flag=='g') {
		if ((setGradient) || (setTilesize)) {
			setGradient=false;
            setTilesize=false;
			SDLinterface::display_lock=false;
		} else if (!SDLinterface::display_lock) {
			setGradient=true;
            setTilesize=false;
			SDLinterface::display_lock=true;
		}
	} else if ((setGradient) || (setTilesize)) {
		switch(flag) {
			case SDLK_LEFT:
                if (setGradient) {
                    gradient--;
                    if (gradient<0) gradient=0;
                } else if (setTilesize) {
                    tile_index--;
                    if (tile_index<0) tile_index=0;
                    tile_size = tile_sizes[tile_index];
               }
				break;
			case SDLK_RIGHT:
                if (setGradient) {
                    gradient++;
                    if (gradient>64) gradient=64;
                } else if (setTilesize) {
                    tile_index++;
                    if (tile_index==tile_count) tile_index=tile_count-1;
                    tile_size = tile_sizes[tile_index];
                }
                break;
            case SDLK_UP:
            case SDLK_DOWN:
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
}

void FrameThresholder::drawGUI(SDL_Surface *display) {
	unsigned char* disp = (unsigned char*)(display->pixels);

    char displayText[64];
	int settingValue ;
	int maxValue;
	
    int x_offset=width/2-128;
    int y_offset=height-100;

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
  
    FontTool::drawText(x_offset+128-(FontTool::getTextWidth(displayText)/2),y_offset-FontTool::getFontHeight(),displayText,display);
    
	// draw the border
	for (int i=x_offset;i<(x_offset+256);i++) {
		int pixel=(width*y_offset+i)*4+1;
		disp[pixel]=disp[pixel+2]=255;
		pixel=(width*(y_offset+25)+i)*4+1;
		disp[pixel]=disp[pixel+2]=255;
	}
	for (int i=y_offset;i<(y_offset+25);i++) {
		int pixel=(width*i+x_offset)*4+1;
		disp[pixel]=disp[pixel+2]=255;
		pixel=(width*i+x_offset+256)*4+1;
		disp[pixel]=disp[pixel+2]=255;
	}
	
	// draw the bar
	int xpos = (int)(254*((float)settingValue/(float)maxValue));
	for (int i=x_offset+2;i<=(x_offset+xpos);i++) {
		for (int j=y_offset+2;j<=(y_offset+23);j++) {
			int pixel=(width*j+i)*4+1;
			disp[pixel]=disp[pixel+2]=255;
		}
	}
	
	
}

