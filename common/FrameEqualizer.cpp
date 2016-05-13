/*  reacTIVision tangible interaction framework
    FrameEqualizer.cpp
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

#include "FrameEqualizer.h"

bool FrameEqualizer::init(int w, int h, int sb, int db) {
	
	FrameProcessor::init(w,h,sb,db);
	
	if (sb!=1) {
		printf("source buffer must be grayscale");
		return false;
	}

	help_text.push_back( "FrameEqualizer:");
	help_text.push_back( "   e - toggle frame equalizer");
	help_text.push_back( "   SPACE - activate frame equalizer");
	
	average = 0;
	int size = width*height;
	pointmap = new unsigned char[size];
	for (int i=0;i<size;i++) pointmap[i] = 0;
	return true;
}

void FrameEqualizer::process(unsigned char *src, unsigned char *dest, SDL_Surface *display) {

    
    //long start_time = SDLinterface::currentTime();
	
	if (equalize) {
		
		unsigned char *map = pointmap;
		int e = 0;
		for (int i=width*height;i>0;i--) {
			e = *src - (*map++ - average);
			if (e & (~255)) { e < 0 ? *src++ = 0 : *src++ = 255; }
			else *src++ = (unsigned char)e;
		}
		
		/*unsigned char *map = pointmap;
		 for (int i=width*height;i>0;i--) {
			int e = *src-*map++;
			if (e < 0) *src++ = 0;
			else *src++ = (unsigned char)e;
		 }*/
		
	} else if (calibrate) {
		
		unsigned int sum = 0;
		for (int x=width/2-5;x<width/2+5;x++) {
			for (int y=height/2-5;y<height/2+5;y++) {
				sum+=src[y*width+x];
			}
		}
		
		average = (unsigned char)(sum/100);
		memcpy(pointmap,src,width*height);
		
		/*int size = width*height;
		 unsigned char *map = pointmap;
		 for (int i=size;i>0;i--) {
			int e = *src++-8;
			if (e < 0) *map++ = 0;
			else *map++ = (unsigned char)e;
		 }*/
		
		calibrate = false;
		equalize = true;
	}
	
     /*long equalize_time = SDLinterface::currentTime() - start_time;
     float latency = (equalize_time/100.0f)/10.0f;
     std::cout << "equalizer latency: " << latency << "ms" << std::endl;*/
}

void FrameEqualizer::toggleFlag(int flag) {

	if (flag=='e') {
		equalize=!equalize;
		calibrate=false;
	}

	if (flag==' ') {
		equalize=false;
		calibrate=true;
	}

}


