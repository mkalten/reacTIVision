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

#include "FiducialFinder.h"
#include "FiducialObject.h"
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
	grid_size_x = 7;
	if (((float)width/height) > 1.3) grid_size_x +=2;
	if (((float)width/height) > 1.7) grid_size_x +=2;
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
	
	//ui->displayMessage("computing distortion matrix ...");
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

bool FiducialFinder::toggleFlag(unsigned char flag, bool lock) {
	
	if ((flag==KEY_R) && (!calibration) && (!empty_grid)) {
		if (!show_grid) {
			show_grid = true;
			prevMode = ui->getDisplayMode();
			ui->setDisplayMode(DEST_DISPLAY);
		} else {
			show_grid = false;
			ui->setDisplayMode(prevMode);
		}
	} else if (flag==KEY_C) {
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
	} else if (flag==KEY_V) {
		if (tuioManager->isVerbose()) tuioManager->setVerbose(false);
		else tuioManager->setVerbose(true);
	} else if (flag==KEY_I) {
		if (currentSetting != INV_NONE) {
			currentSetting = INV_NONE;
			show_settings = false;
			return false;
		} else if (!lock) {
			currentSetting = INV_XPOS;
			show_settings = true;
			return true;
		}
	} else if ( currentSetting != INV_NONE ) {
		switch(flag) {
			case KEY_LEFT:
				if (currentSetting==INV_XPOS) tuioManager->setInvertXpos(false);
				else if (currentSetting==INV_YPOS) tuioManager->setInvertYpos(false);
				else if (currentSetting==INV_ANGLE) tuioManager->setInvertAngle(false);
				break;
			case KEY_RIGHT:
				if (currentSetting==INV_XPOS) tuioManager->setInvertXpos(true);
				else if (currentSetting==INV_YPOS) tuioManager->setInvertYpos(true);
				else if (currentSetting==INV_ANGLE) tuioManager->setInvertAngle(true);
				break;
			case KEY_UP:
				currentSetting--;
				if (currentSetting == INV_NONE) currentSetting = INV_ANGLE;
				break;
			case KEY_DOWN:
				currentSetting++;
				if (currentSetting > INV_ANGLE) currentSetting = INV_XPOS;
				break;
		}
	}
	return lock;
}

void FiducialFinder::displayControl() {
	
	char displayText[64];
	int settingValue = 0;
	int maxValue = 1;
	
	switch (currentSetting) {
		case INV_NONE: return;
		case INV_XPOS: {
			sprintf(displayText,"invert X-axis %d",tuioManager->getInvertXpos());
			settingValue = tuioManager->getInvertXpos();
			break;
		}
		case INV_YPOS: {
			sprintf(displayText,"invert Y-axis %d",tuioManager->getInvertYpos());
			settingValue = tuioManager->getInvertYpos();
			break;
		}
		case INV_ANGLE: {
			sprintf(displayText,"invert angle %d",tuioManager->getInvertAngle());
			settingValue = tuioManager->getInvertAngle();
			break;
		}
	}
	
	ui->drawText(17,14,"I          - exit inversion configuration");
	ui->displayControl(displayText, 0, maxValue, settingValue);
}

void FiducialFinder::drawObject(int id, float xpos, float ypos, int state)
{
	if (ui==NULL) return;
	if (ui->getDisplayMode()==NO_DISPLAY) return;
	
	char id_str[8];
	if(id>=0) sprintf(id_str,"%d",id);
	else if (id==FINGER_ID)sprintf(id_str,"F");
	else if (id==BLOB_ID)sprintf(id_str,"B");
	
	switch (state) {
		case FIDUCIAL_FOUND:
			ui->setColor(0,255,0);
			break;
		case FIDUCIAL_FUZZY:
			ui->setColor(255,255,0);
			break;
		case FIDUCIAL_ROOT:
			ui->setColor(255,0,0);
			break;
	}
	
	int x = (int)floor(xpos * width + 0.5f);
	int y = (int)floor(ypos * height + 0.5f);
	
	ui->drawText(x,y,id_str);
	
	ui->drawLine(x+1, y, x+2, y);
	ui->drawLine(x-1, y, x-2, y);
	ui->drawLine(x, y+1, x, y+2);
	ui->drawLine(x, y-1, x, y-2);
}

void FiducialFinder::drawGrid(unsigned char *src, unsigned char *dest) {
	
	if (ui==NULL) return;
	if (ui->getDisplayMode()==NO_DISPLAY) return;
	
	int size = width*height-1;

	// delete the dest image
	for (int i=size;i>0;i--) dest[i]=0;

	// distort the image
	for (int i=size;i>0;i--) {
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
	ui->setColor(0, 0, 255);
	if (grid_size_x>7) ui->drawEllipse(width/2, height/2,width,height);
	if (grid_size_x>9) ui->drawEllipse(width/2, height/2,width-cell_width*2,height);
	ui->drawEllipse(width/2, height/2,cell_width*6,cell_height*6);
	ui->drawEllipse(width/2, height/2,cell_width*4,cell_height*4);
	ui->drawEllipse(width/2, height/2,cell_width*2,cell_height*2);

	// draw the horizontal lines
	ui->setColor(0, 255, 0);
	for (int i=0;i<grid_size_y;i++) {
		float start_x = 0;
		float start_y = i*cell_height;
		float end_x = (grid_size_x-1)*cell_width;
		float end_y = start_y;
		
		ui->drawLine(start_x, start_y,end_x,end_y);
	}
	
	// draw the vertical lines
	for (int i=0;i<grid_size_x;i++) {
		
		float start_x = i*cell_width;
		float start_y = 0;
		float end_x = start_x;
		float end_y = (grid_size_y-1)*cell_height;
		
		ui->drawLine(start_x, start_y,end_x,end_y);
	}
	
}

