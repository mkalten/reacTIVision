/*  reacTIVision tangible interaction framework
    FiducialFinder.cpp
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

#include "FiducialFinder.h"
#include "CalibrationGrid.h"

bool FiducialFinder::init(int w, int h, int sb ,int db) {
	FrameProcessor::init(w,h,sb,db);

	help_text.push_back( "FiducialFinder:");
	help_text.push_back( "   i - invert x-axis, y-axis or angle");

	show_grid=false;
	dmap = new ShortPoint[height*width];
	computeGrid();

	return true;
}

void FiducialFinder::computeGrid() {

	// load the distortion grid
	grid_size_x = 9;
	grid_size_y = 7;
	cell_width = width/(grid_size_x-1);
	cell_height = height/(grid_size_y-1);

	CalibrationGrid grid(grid_size_x,grid_size_y);
	grid.Load(grid_config);

	// we do not calculate the matrix if the grid is not configured
	if (grid.IsEmpty()) {
		empty_grid = true;
		for (int y=0;y<height;y++) {
			for (int x=0;x<width;x++) {
				dmap[y*width+x].x = x;
				dmap[y*width+x].y = y;
			}
		}
		return;
	} else empty_grid = false;

	//if(msg_listener) msg_listener->displayMessage("computing distortion matrix ...");
	//if(msg_listener) msg_listener->displayMessage("");

	// reset the distortion matrix
	for (int y=0;y<height;y++) {
		for (int x=0;x<width;x++) {
			dmap[y*width+x].x = 0;
			dmap[y*width+x].y = 0;
		}
	}

	// calculate the distortion matrix
	for (float y=0;y<height;y+=0.5) {
		for (float x=0;x<width;x+=0.5) {

			// get the displacement
			GridPoint new_point =  grid.GetInterpolated(x/cell_width,y/cell_height);

			// apply the displacement
			short dx = (short)floor(x+0.5f+new_point.x*cell_width);
			short dy = (short)floor(y+0.5f+new_point.y*cell_height);

			int pixel =  dy*width+dx;
			if ((dx>=0) && (dx<width) && (dy>=0) && (dy<height)) {
				dmap[pixel].x = (short)x;
				dmap[pixel].y = (short)y;
			}
		}
	}

}

void FiducialFinder::toggleFlag(int flag) {

	if ((flag=='r') && (!calibration) && (!empty_grid)) {
		if (!show_grid) {
			show_grid = true;
			if(msg_listener) {
				prevMode = msg_listener->getDisplayMode();
				msg_listener->setDisplayMode(msg_listener->DEST_DISPLAY);
			}
		} else {
			show_grid = false;
			if(msg_listener) msg_listener->setDisplayMode(prevMode);
		}
	} else if (flag=='c') {
		if(!calibration) {
			calibration=true;
			show_grid=false;
			for (int y=0;y<height;y++) {
				for (int x=0;x<width;x++) {
					dmap[y*width+x].x = x;
					dmap[y*width+x].y = y;
				}
			}
		}
		else {
			calibration = false;
			computeGrid();
		}
	} else if (flag=='i') {
		if (currentSetting != INV_NONE) {
			currentSetting = INV_NONE;
			show_settings = false;
			SDLinterface::display_lock = false;
		} else if (!SDLinterface::display_lock) { 
			currentSetting = INV_XPOS;
			show_settings = true;
			SDLinterface::display_lock = true;
		}
	} else if ( currentSetting != INV_NONE ) {
		switch(flag) {
			case SDLK_LEFT: 
				if (currentSetting==INV_XPOS) message_server->setInvertX(false);
				else if (currentSetting==INV_YPOS) message_server->setInvertY(false);
				else if (currentSetting==INV_ANGLE) message_server->setInvertA(false);
				break;
			case SDLK_RIGHT:
				if (currentSetting==INV_XPOS) message_server->setInvertX(true);
				else if (currentSetting==INV_YPOS) message_server->setInvertY(true);
				else if (currentSetting==INV_ANGLE) message_server->setInvertA(true);
				break;
			case SDLK_UP: 
				currentSetting--;
				if (currentSetting == INV_NONE) currentSetting = INV_ANGLE;
				break;
			case SDLK_DOWN:
				currentSetting++;
				if (currentSetting > INV_ANGLE) currentSetting = INV_XPOS;
				break;
		}
	}
	
}

void FiducialFinder::drawGUI(SDL_Surface *display) {
	unsigned char* disp = (unsigned char*)(display->pixels);
	
	char displayText[64];
	int maxValue = 1;
	int settingValue = 0;
		
	switch (currentSetting) {
		case INV_NONE: return;
		case INV_XPOS: {
			sprintf(displayText,"invert X-axis %d",message_server->getInvertX());
			settingValue = message_server->getInvertX();		
			break;
		}
		case INV_YPOS: {
			sprintf(displayText,"invert Y-axis %d",message_server->getInvertY());
			settingValue = message_server->getInvertY();
			break;
		}
		case INV_ANGLE: {
			sprintf(displayText,"invert angle %d",message_server->getInvertA());
			settingValue = message_server->getInvertA();
			break;
		}			
	}
	
	int x_offset=width/2-128;
	int y_offset=height-100;
	
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

void FiducialFinder::sendTuioMessages() {

		// get the current set of objects on the table
		// we do not only use the raw objects retrieved from the frame
		// but also the remaining (filtered) objects from our fiducialObject list
		bool aliveMessage = false;
		int aliveSize = 0;
		int *aliveList = new int[fiducialList.size()];
		
		for(std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); ) {

			std::string remove_message = fiducial->checkRemoved();
			if (remove_message!="") {
				if(msg_listener) msg_listener->setMessage(remove_message);
				
				fiducial = fiducialList.erase(fiducial);
				// send a single alive message if any
				// of the objects reported to have been removed
				aliveMessage = true;
			} else {
				aliveList[aliveSize]=fiducial->session_id;
				//aliveList[aliveSize]=pos->classId;
				aliveSize++;
				fiducial++;
			} 
		}

		totalframes++;
		long fseq = totalframes;
	
		// after a certain number of frames we should send the redundant alive message
		if ((aliveSize>0) || (totalframes%30==0)) aliveMessage = true;
		if ((aliveSize==0) && (totalframes%30==0)) fseq = -1;
		
		// add the alive message
		if (aliveMessage) {
			tuio_server->addObjAlive(aliveList, aliveSize);
		
			char unsent = 0;
			// send a set message (in case anything changed compared to the last frame)
			for(std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
				if (!tuio_server->freeObjSpace()) {
					tuio_server->addObjSeq(fseq);
					tuio_server->sendObjMessages();
				}
				
				std::string set_message = fiducial->addSetMessage(tuio_server);
				if(set_message!="" && msg_listener) msg_listener->setMessage(set_message);
		
				if (fiducial->unsent>unsent) unsent = fiducial->unsent;
			}
	
			// fill the rest of the packet with unchanged objects
			// sending the least sent ones first
			while (unsent>0) {
				for(std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
					
					if (fiducial->unsent==unsent) {
						if (tuio_server->freeObjSpace()) { 
							fiducial->redundantSetMessage(tuio_server);
						} else goto send;
					}
				}
				unsent--;
			}
				
			send:
			// finally send the packet
			tuio_server->addObjSeq(fseq);
			tuio_server->sendObjMessages();
			//if (fseq>0) std::cout << "sent frame " << totalframes << " at " << SDLinterface::currentTime() << std::endl; 
		}
		delete[] aliveList;
}

void FiducialFinder::sendMidiMessages() {

		for(std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end();) {
			std::string remove_message = fiducial->checkRemoved();			
			if(remove_message!="") {
				midi_server->sendRemoveMessage(fiducial->fiducial_id);
				if(msg_listener) msg_listener->setMessage(remove_message);
				fiducial = fiducialList.erase(fiducial);
			} else {
				std::string set_message = fiducial->addSetMessage(midi_server);
				if(set_message!="" && msg_listener) msg_listener->setMessage(set_message);
				fiducial++;
			}
		}
		totalframes++;
}

void FiducialFinder::finish() {
	
	// on exit we have to send an empty alive message to clear the scene
	if (tuio_server) {
		tuio_server->addObjAlive(NULL, 0);
		tuio_server->addObjSeq(-1);
		tuio_server->sendObjMessages();
		tuio_server->addCurAlive(NULL, 0);
		tuio_server->addCurSeq(-1);
		tuio_server->sendCurMessages();
	} else if (midi_server){
		midi_server->sendNullMessages();
	}
}


void FiducialFinder::drawObject(int id, int xpos, int ypos, SDL_Surface *display, int state)
{
	if (msg_listener->getDisplayMode()==SDLinterface::NO_DISPLAY) return;
	char id_str[8];
	if(id>=0) sprintf(id_str,"%d",id);
	else sprintf(id_str,"F");
	FontTool::drawText(xpos,ypos,id_str,display);
	
	int pixel = 0;
	unsigned char* disp = (unsigned char*)(display->pixels);
	for (int i=1;i<3;i++) {
		pixel=4*(ypos*width+(xpos+i));
		if ((pixel>=0) && (pixel<width*height*3)) disp[pixel+state]=disp[pixel+3]=255;
		pixel=4*(ypos*width+(xpos-i));
		if ((pixel>=0) && (pixel<width*height*3)) disp[pixel+state]=disp[pixel+3]=255;
		pixel=4*((ypos+i)*width+xpos);
		if ((pixel>=0) && (pixel<width*height*3)) disp[pixel+state]=disp[pixel+3]=255;
		pixel=4*((ypos-i)*width+xpos);
		if ((pixel>=0) && (pixel<width*height*3)) disp[pixel+state]=disp[pixel+3]=255;
	}
}

void FiducialFinder::drawGrid(unsigned char *src, unsigned char *dest, SDL_Surface *display) {

	unsigned char* disp = (unsigned char*)(display->pixels);
	int length = width*height;
	int offset = (width-height)/2;
	int half = height/2;

	// delete the dest image
	for (int i=0;i<length;i++) dest[i]=0;

	// distort the image
	for (int i=length-1;i>=0;i--) {
		if( (dmap[i].x>=0) && ( dmap[i].x<width) && (dmap[i].y>=0) && ( dmap[i].y<height) )
			dest[dmap[i].y*width+dmap[i].x] = src[i];
	}

	// interpolate empty points
	for (int i=0;i<height;i++) {
		for (int j=0;j<width;j++) {
 			if (dest[i*width+j]==0) {
			int sum = 0;
			int pixel = 0;
			for (int ii=i-1;ii<=i+1;ii++) {
				for (int jj=j-1;jj<=j+1;jj++) {
					if ((ii==i) && (jj==j)) continue;
					if ((ii>=0) && (ii<height) && (jj>=0) && (jj<width) && (dest[ii*width+jj]>0)) {
						pixel+=dest[ii*width+jj];
						sum++;
						}
					}
				}
				if(sum>0) dest[i*width+j] = (unsigned char)(pixel/sum);
			}
		}
	}
	
	// draw the circles
	for (double a=-M_PI;a<M_PI;a+=0.005) {

		// ellipse
		float x = width/2+cos(a)*width/2;
		float y = half+sin(a)*half;
		if ((x>=0) && (x<width) && (y>=0) && (y<height)) {
			int pixel = 4*((int)y*width+(int)x);
			disp[pixel] = disp[pixel+3] = 255;
		}
		
		// outer circle
		x = offset+half+cos(a)*half;
		y = half+sin(a)*half;
		if ((x>=0) && (x<width) && (y>=0) && (y<height)) {
			int pixel = 4*((int)y*width+(int)x);
			disp[pixel] = disp[pixel+3] = 255;
		}

		// middle circle
		x = offset+half+cos(a)*(half-cell_width);
		y = half+sin(a)*(half-cell_height);
		if ((x>=0) && (x<width) && (y>=0) && (y<height)) {
			int pixel = 4*((int)y*width+(int)x);
			disp[pixel] = disp[pixel+3] = 255;
		}

		// inner circle
		x = offset+half+cos(a)*cell_width;
		y = half+sin(a)*cell_height;
		if ((x>=0) && (x<width) && (y>=0) && (y<height)) {
			int pixel = 4*((int)y*width+(int)x);
			disp[pixel] = disp[pixel+3] = 255;
		}

	}
			
	// draw the horizontal lines
	for (int i=0;i<grid_size_y;i++) {
		float start_x = 0;
		float start_y = i*cell_width;
		float end_x = (grid_size_x-1)*cell_height;
		float end_y = start_y;
				
		for (float lx=start_x;lx<end_x;lx++) {
			float ly = end_y - (end_y-start_y)/(end_x-start_x)*(end_x-lx);
			int pixel = 1+4*((int)ly*width+(int)lx);
			if ((pixel>=0) && (pixel<length*4)) {
				disp[pixel] = disp[pixel+2] = 255;
				disp[pixel-1]=0;
			}
		}
	}

	// draw the vertical lines
	for (int i=0;i<grid_size_x;i++) {

		float start_x = i*cell_width;
		float start_y = 0;
		float end_x = start_x;
		float end_y = (grid_size_y-1)*cell_height;
				
		for (float ly=start_y;ly<end_y;ly++) {
			float lx = end_x - (end_x-start_x)/(end_y-start_y)*(end_y-ly);	
			int pixel = 1+4*((int)ly*width+(int)lx);
			if ((pixel>=0) && (pixel<length*4)) {
				disp[pixel] = disp[pixel+2] = 255;
				disp[pixel-1]=0;
			}
		}
	}

}

