/*  reacTIVision tangible interaction framework
    FidtrackFinder.cpp
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

#include "FidtrackFinder.h"
#include <sstream>

bool FidtrackFinder::init(int w, int h, int sb, int db) {

	FiducialFinder::init(w,h,sb,db);

	if (db!=1) {
		printf("target buffer must be grayscale\n");
		return false;
	}

	help_text.push_back( "FidtrackFinder:");
	help_text.push_back( "   f - set finger size & sensitivity");

	if (strcmp(tree_config,"none")!=0) {
		initialize_treeidmap_from_file( &treeidmap, tree_config );
	} else initialize_treeidmap( &treeidmap );

	initialize_fidtrackerX( &fidtrackerx, &treeidmap, dmap);
	initialize_segmenter( &segmenter, width, height, treeidmap.max_adjacencies );
	//finger_buffer = new unsigned char[64*64];

	average_leaf_size = 4.0f;
	average_fiducial_size = 48.0f;

	setFingerSize = false;
	setFingerSensitivity = false;

	return true;
}


int FidtrackFinder::check_finger(RegionX *finger, unsigned char *image, unsigned char *display) {

	int buffer_size = width*height;
	int mask_size = finger->width;
	if (finger->height>finger->width) mask_size = finger->height;

	float outer_radius = mask_size/2.0f;
	float inner_radius = mask_size/3.0f;

	int rounded_outer_radius = floor(outer_radius+0.5f);
	int rounded_inner_radius = ceil(inner_radius+0.5f);

	float xratio = (float)mask_size/(float)finger->width;
	float yratio = (float)mask_size/(float)finger->height;
	short outer_error = 0;
	short inner_error = 0;

	for (int x=0;x<mask_size;x++) {
		for (int y=0;y<mask_size;y++) {

			int px = finger->left+(int)floor(x/xratio+0.5f);
			int py = finger->top +(int)floor(y/yratio+0.5f);

			int finger_pixel = py*width+px;
			if ((finger_pixel>=buffer_size) || (finger_pixel<0)) return -1;

			float dx = x-outer_radius+0.5f;
			float dy = y-outer_radius+0.5f;
			int dist = floor(sqrtf(dx*dx+dy*dy)+0.5f);

			//if ((x==0) || (x==mask_size-1)) display[finger_pixel*3+1]=255; 
			//if ((y==0) || (y==mask_size-1)) display[finger_pixel*3+1]=255; 
			//if ((x==finger->width/2) && (y==finger->height/2)) display[finger_pixel*3+1]=255; 

			if (( dist> rounded_outer_radius) && (image[finger_pixel]==255)) { outer_error++; } //display[finger_pixel*3+2]=255; }
			else if (( dist< rounded_inner_radius) && (image[finger_pixel]==0)) { inner_error++; } //display[finger_pixel*3]=255; }
		}
	}

	short outer_max_error = (short)floor((mask_size*mask_size)-(outer_radius*outer_radius*M_PI)+0.5f);
	short inner_max_error = (short)floor(inner_radius*inner_radius*M_PI+0.5f);

	//std::cout << "finger: " << finger->left << " " << finger->top << " " << finger->width << " " << finger->height << std::endl;
	//std::cout << "error: " << outer_error << " " << inner_error << std::endl;
	//std::cout << "max: " << outer_max_error << " " << inner_max_error << std::endl;

	if ((outer_error>outer_max_error/4*finger_sensitivity) || (inner_error>inner_max_error/6*finger_sensitivity)) return -1;
	if ((outer_error>outer_max_error/6*finger_sensitivity) || (inner_error>inner_max_error/9*finger_sensitivity)) return 0;	
	return 1;
}

/*
int FidtrackFinder::check_finger(RegionX *finger, unsigned char *image, unsigned char *display) {

	int mask_size = finger->width;
	if (finger->height>finger->width) mask_size = finger->height;
	float outer_radius = floor(mask_size/2.0f);
	float inner_radius = ceil(mask_size/3.0f);

	float xratio = (float)mask_size/(float)finger->width;
	float yratio = (float)mask_size/(float)finger->height;
	short outer_error = 0;
	short inner_error = 0;
	int finger_buffer_size = mask_size*mask_size;
	
	for (int i=0;i<finger_buffer_size;i++) finger_buffer[i] = 0;
	
	// this should actually be done without creating the bitmap
	Span *span = finger->span;
	while (span) {
		for(int i=span->start;i<=span->end;i++) {
			int fy = floor(i/width) - finger->top;
			int fx = i%width - finger->left;
			
			int fp = fy*finger->width+fx;
			finger_buffer[fp] = 255;
		}
		span = span->next;
	}	

	//calculate error using the clean finger bitmap
	for (int x=0;x<mask_size;x++) {
		for (int y=0;y<mask_size;y++) {

			int px = (int)(floorf((float)x/xratio+0.5f));
			int py = (int)(floorf((float)y/yratio+0.5f));

			int finger_pixel = py*finger->width+px;
			if ((finger_pixel>=finger_buffer_size) || (finger_pixel<0)) return -1;

			int dx = x-outer_radius;
			int dy = y-outer_radius;
			float dist = sqrtf(dx*dx+dy*dy);
			
			if ((floor(dist)>outer_radius) && (finger_buffer[finger_pixel]==255)) outer_error++; 
			else if ((dist<inner_radius) && (finger_buffer[finger_pixel]==0)) inner_error++;


			int dbg_px = finger->left+(int)floor(x/xratio+0.5);
			int dbg_py = finger->top +(int)floor(y/yratio+0.5);
			
			int dbg_finger_pixel = dbg_py*width+dbg_px;
			if ((dbg_finger_pixel>=width*height) || (dbg_finger_pixel<0)) {
				return -1;
			}
			
			if ((x==0) || (x==mask_size-1)) display[dbg_finger_pixel*3+1]=255;
			if ((y==0) || (y==mask_size-1)) display[dbg_finger_pixel*3+1]=255;
			if ((x==outer_radius) && (y==outer_radius)) display[dbg_finger_pixel*3+2]=255;
			
			if ((floor(dist)>outer_radius) && (finger_buffer[finger_pixel]==255)) display[dbg_finger_pixel*3+2]=255;
			else if ((dist<inner_radius) && (finger_buffer[finger_pixel]==0)) display[dbg_finger_pixel*3]=255;
			
		} 
	}
 	
	float outer_max_error = (int)((mask_size*mask_size)-(outer_radius*outer_radius*M_PI));
	float inner_max_error = (int)(inner_radius*inner_radius*M_PI);

//	std::cout << "finger: " << finger->left << " " << finger->top << " " << finger_width << " " << finger_height << std::endl;
//	std::cout << "error: " << outer_error << " " << inner_error << std::endl;
//	std::cout << "max: " << outer_max_error << " " << inner_max_error << std::endl;

	if ((outer_error>outer_max_error/6.0f*finger_sensitivity) || (inner_error>inner_max_error/9.0f*finger_sensitivity)) return -1;
	if ((outer_error>outer_max_error/9.0f*finger_sensitivity) || (inner_error>inner_max_error/12.0f*finger_sensitivity)) return 0;
	return 1;
}
*/


void FidtrackFinder::reset() {
	fiducialList.clear();
	fingerList.clear();
}

void FidtrackFinder::toggleFlag(int flag) {
	
	FiducialFinder::toggleFlag(flag);
	
	if (flag=='f') {
			if (setFingerSize || setFingerSensitivity) {
				setFingerSize = false;
				setFingerSensitivity = false;
				show_settings = false;
				SDLinterface::display_lock = false;
			} else if (!SDLinterface::display_lock) {
				setFingerSize = true;
				show_settings = true;
				SDLinterface::display_lock = true;
			}
	} else if (setFingerSize || setFingerSensitivity) {
		switch(flag) {
			case SDLK_LEFT: 
				if (setFingerSize) {
					average_finger_size--;
					if (average_finger_size<0) average_finger_size=0;
					if (average_finger_size==0) detect_finger=false;
				} else {
					finger_sensitivity-=0.05f;
					if (finger_sensitivity<0) finger_sensitivity=0;
				}
			break;
			case SDLK_RIGHT:
				if (setFingerSize) {
					average_finger_size++;
					if (average_finger_size>64) average_finger_size=64;
					detect_finger=true;
				} else {
					finger_sensitivity+=0.05f;
					if (finger_sensitivity>2) finger_sensitivity=2.0f;
				}
				break;
			case SDLK_UP: 
			case SDLK_DOWN:
				if (setFingerSize) {
					setFingerSize=false;
					setFingerSensitivity=true;
				} else {
					setFingerSize=true;
					setFingerSensitivity=false;
				}
				break;
		}
	}
	
}

void FidtrackFinder::drawGUI(SDL_Surface *display) {
	
	FiducialFinder::drawGUI(display);

	unsigned char* disp = (unsigned char*)(display->pixels);

	char displayText[64]; 
	int settingValue = 0;
	int maxValue = 0;

	if (setFingerSize) {

		// draw the finger
		if( average_finger_size>0) {
			int offset = (width-height)/2;
			int half = height/2;
			for (double a=-M_PI;a<M_PI;a+=0.02) {
				
				double x = offset+half+cos(a)*average_finger_size/2.0f;
				double y = half+sin(a)*average_finger_size/2.0f;
				
				if ((x>=0) && (x<width) && (y>=0) && (y<height)) {
					int pixel = (int)y*width+(int)x;
					disp[pixel*4] = disp[pixel*4+3] = 255;
				}
			}
		}
		
		sprintf(displayText,"finger size %d",average_finger_size);
		settingValue = average_finger_size;
		maxValue = 64;
	} else if (setFingerSensitivity) {
		sprintf(displayText,"finger sensitivity %d",(int)floor(finger_sensitivity*100+0.5f));
		settingValue = (int)floor(finger_sensitivity*100+0.5f);
		maxValue = 200;
	} else return;
	
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

void FidtrackFinder::process(unsigned char *src, unsigned char *dest, SDL_Surface *display) {

/*
	#ifdef WIN32
		long start_time = GetTickCount();
	#else
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv,&tz);
		long start_time = (tv.tv_sec*1000000)+(tv.tv_usec);
	#endif
*/
    
    //long start_time = SDLinterface::currentTime();

	// segmentation
	step_segmenter( &segmenter, dest );
	// fiducial recognition
	int fid_count = find_fiducialsX( fiducials, MAX_FIDUCIAL_COUNT,  &fidtrackerx, &segmenter, width, height);
    
    //long segment_time = SDLinterface::currentTime() - start_time;
    //std::cout << "segment latency: " << (segment_time/100.0f)/10.0f << "ms" << std::endl;
    //start_time = SDLinterface::currentTime();

	float total_leaf_size = 0.0f;
	float total_fiducial_size = 0.0f;
	int valid_fiducial_count = 0;

	// process found symbols
	for(int i=0;i< fid_count;i++) {
		if ( fiducials[i].id >=0 ) {
			valid_fiducial_count ++;
			total_leaf_size += fiducials[i].leaf_size;
			total_fiducial_size += fiducials[i].root_size;
		}
		
		FiducialObject *existing_fiducial = NULL;
		// update objects we had in the last frame
		// also check if we have an ID/position conflict
		// or correct an INVALID_FIDUCIAL_ID if we had an ID in the last frame
		for (std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
	
			float distance = fiducial->distance(fiducials[i].x,fiducials[i].y);
	
			if (fiducials[i].id==fiducial->fiducial_id)  {
				// find and match a fiducial we had last frame already ...
				if(!existing_fiducial) {
					existing_fiducial = &(*fiducial);

					for (int j=0;j<fid_count;j++) {
						if ( (i!=j) && (fiducials[j].id==fiducial->fiducial_id) && (fiducial->distance(fiducials[j].x,fiducials[j].y)<distance)) {
							//check if there is another fiducial with the same id closer
							existing_fiducial = NULL;
							break;
						}
					}	
										
					if (existing_fiducial!=NULL) {
					for (std::list<FiducialObject>::iterator test = fiducialList.begin(); test!=fiducialList.end(); test++) {
						FiducialObject *test_fiducial = &(*test);
						if ( (test_fiducial!=existing_fiducial) && (test_fiducial->fiducial_id==existing_fiducial->fiducial_id) && (test_fiducial->distance(fiducials[i].x,fiducials[i].y)<distance)) {
							//check if there is another fiducial with the same id closer
							existing_fiducial = NULL;
							break;
						}
					}}
				} /*else if (distance<existing_fiducial->distance(fiducials[i].x,fiducials[i].y)) {
						existing_fiducial = &(*fiducial);
					// is this still necessary?
				} */

			} else if ((distance<average_fiducial_size/1.2) && (abs(fiducials[i].node_count-fiducial->node_count)<=FUZZY_NODE_RANGE)) {
				// do we have a different ID at the same place?
				// this should correct wrong or invalid fiducial IDs
				// assuming that between two frames
				// there can't be a rapid exchange of two symbols
				// at the same place
				
				for (int j=0;j<fid_count;j++) {
					if ( (i!=j) && (fiducial->distance(fiducials[j].x,fiducials[j].y)<distance)) goto fiducialList_loop_end;
				}	
				
				if (fiducials[i].id==INVALID_FIDUCIAL_ID) {
					//printf("corrected invalid ID to %d (%ld)\n", fiducial->fiducial_id,fiducial->session_id);
					
					//two pixel threshold since missing/added leaf nodes result in a slightly different position
					float dx = abs(fiducial->getX() - fiducials[i].x);
					float dy = abs(fiducial->getY() - fiducials[i].y);
					if ((dx<2.0f) && (dy<2.0f)) {
						fiducials[i].x = (short int)fiducial->getX();
						fiducials[i].y = (short int)fiducial->getY();
					}
					
					fiducials[i].angle=fiducial->getAngle();
					fiducials[i].id=fiducial->fiducial_id;
					fiducial->state = FIDUCIAL_INVALID;
					drawObject(fiducials[i].id,(int)(fiducials[i].x),(int)(fiducials[i].y),display,0);
				} else /*if (fiducials[i].id!=fiducial->fiducial_id)*/ {

					if (!fiducial->checkIdConflict(session_id,fiducials[i].id)) {
						//printf("corrected wrong ID from %d to %d (%ld)\n", fiducials[i].id,fiducial->fiducial_id,fiducial->session_id);
						fiducials[i].id=fiducial->fiducial_id;
					} else {
						session_id++;
					}
				}
				
				existing_fiducial = &(*fiducial);
				break;
			}
			
			fiducialList_loop_end:;
		}
		
		if  (existing_fiducial!=NULL) {
			// just update the fiducial from last frame ...
			existing_fiducial->update(fiducials[i].x,fiducials[i].y,fiducials[i].angle,fiducials[i].root_size,fiducials[i].leaf_size);
			if(existing_fiducial->state!=FIDUCIAL_INVALID) drawObject(existing_fiducial->fiducial_id,(int)(existing_fiducial->getX()),(int)(existing_fiducial->getY()),display,1);
		} else if  (fiducials[i].id!=INVALID_FIDUCIAL_ID) {
			// add the newly found object
			session_id++;
			FiducialObject addFiducial(session_id, fiducials[i].id, width, height,fiducials[i].root_colour,fiducials[i].node_count);
			addFiducial.update(fiducials[i].x,fiducials[i].y,fiducials[i].angle,fiducials[i].root_size,fiducials[i].leaf_size);
			drawObject(fiducials[i].id,(int)(fiducials[i].x),(int)(fiducials[i].y),display,1);
			fiducialList.push_back(addFiducial);
			if (midi_server!=NULL) midi_server->sendAddMessage(fiducials[i].id);
			if (msg_listener) {
				std::stringstream add_message;
				add_message << "add obj " << session_id << " " << fiducials[i].id;
				msg_listener->setMessage(add_message.str());
			}
		}
			
		//else drawObject(fiducials[i].id,(int)(fiducials[i].x),(int)(fiducials[i].y),display,0);

	}

	if (valid_fiducial_count>0) {
		average_leaf_size = (average_leaf_size + total_leaf_size/(double)valid_fiducial_count)/2;
		average_fiducial_size = (average_fiducial_size +  total_fiducial_size/(double)valid_fiducial_count)/2;
	}
	//std::cout << "leaf size: " << average_leaf_size << std::endl;
	//std::cout << "fiducial size: " << average_fiducial_size << std::endl;
	//std::cout << "finger size: " << average_finger_size << std::endl;

	float min_object_size = average_fiducial_size - average_fiducial_size/4.0f;
	float max_object_size = average_fiducial_size + average_fiducial_size/4.0f;
	
	float min_finger_size = average_finger_size - average_finger_size/1.75f;
	float max_finger_size = average_finger_size + average_finger_size/1.75f;
	
	// plain object tracking
	int min_region_size = min_finger_size;
	int max_region_size = max_object_size;
	if ((average_finger_size==0) || (min_region_size>min_object_size)) min_region_size = min_object_size;
	int reg_count = find_regionsX( regions, MAX_FIDUCIAL_COUNT, &fidtrackerx, &segmenter, width, height, min_region_size, max_region_size);
	//std::cout << reg_count << std::endl;

	//if (average_fiducial_size>width/4) goto send_messages;
	for(int j=0;j< reg_count;j++) {

		//printf("region: %d %d\n", regions[j].x, regions[j].y);
		//printf("region: %d %d\n", regions[j].width, regions[j].height);

		int diff = abs(regions[j].width - regions[j].height);
		int max_diff = regions[j].width;
		if (regions[j].height > regions[j].width) max_diff = regions[j].height/2.0f;
				
		// plain objects
		if ((regions[j].width>min_object_size) && (regions[j].width<max_object_size) &&
			(regions[j].height>min_object_size) && (regions[j].height<max_object_size) && diff < max_diff) {

			//printf("region: %d %d\n", regions[j].width, regions[j].height);
			FiducialObject *existing_fiducial = NULL;
			for (std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
				float distance = fiducial->distance(regions[j].x,regions[j].y);
				if ((distance<average_fiducial_size/1.5f) && (fiducial->root_colour==regions[j].colour)) {
					existing_fiducial = &(*fiducial);
	
					// check if the fiducial was already found anyway
					for (int i=0;i<fid_count;i++) {
						float dx = fiducials[i].x - regions[j].x;
						float dy = fiducials[i].y - regions[j].y;
						distance = sqrt(dx*dx+dy*dy);
						if ( distance<average_fiducial_size/1.5f) {
							existing_fiducial->setBlobOffset(dx,dy);
							existing_fiducial = NULL;
							break;
						}
					}
					if (existing_fiducial != NULL) break;
				}
			}
			if (existing_fiducial!=NULL) {

				FloatPoint offset = existing_fiducial->getBlobOffset();
				float xpos = regions[j].x + offset.x;
				float ypos = regions[j].y + offset.y;
				float angle = existing_fiducial->getAngle();

				//two pixel threshold since root node blobs do not provide a precise position
				float dx = abs(existing_fiducial->getX() - xpos);
				float dy = abs(existing_fiducial->getY() - ypos);
				if ((dx<2.0f) && (dy<2.0f)) {
					xpos = existing_fiducial->getX();
					ypos = existing_fiducial->getY();
				}
				
				existing_fiducial->update(xpos,ypos,angle,(regions[j].width+regions[j].height)/2,0);
				//printf("region: %d %d\n", regions[j].width, regions[j].height);
				//printf("recovered plain fiducial %d (%ld)\n", existing_fiducial->fiducial_id,existing_fiducial->session_id);
				existing_fiducial->state = FIDUCIAL_REGION;
				drawObject(existing_fiducial->fiducial_id,xpos,ypos,display,2);
			} //else goto plain_analysis;
						
		// plain fingers
		} else if (detect_finger && (regions[j].colour==255) && (regions[j].width>min_finger_size) && (regions[j].width<max_finger_size) && 
			(regions[j].height>min_finger_size) && (regions[j].height<max_finger_size) && diff < max_diff) {

			//check first if the finger is valid
			//printf("candidate: %d %d\n", regions[j].width, regions[j].height);
			int finger_match = check_finger(&regions[j],dest,(unsigned char*)(display->pixels));
			if(finger_match<0) continue;//goto plain_analysis;
			//printf("finger: %d %d\n", regions[j].width, regions[j].height);

			FingerObject *existing_finger = NULL;
			float closest = width;

			// ignore fingers within existing fiducials
			for (std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
				if (fiducial->distance(regions[j].x, regions[j].y)<fiducial->root_size/2) goto region_loop_end;
			}
			
			// attach to existing fingers in the list
			for (std::list<FingerObject>::iterator finger = fingerList.begin(); finger!=fingerList.end(); finger++) {
		
				float distance = finger->distance(regions[j].x,regions[j].y);
				if ((distance<average_finger_size*2) && (distance<closest)){
					existing_finger = &(*finger);
					closest = distance;
					
					//is there another finger closer than that?
					if (distance>average_finger_size/2) {
					for(int k=0;k< reg_count;k++) {
						if(j==k) continue;
						 if ((regions[k].width>min_finger_size) && (regions[k].width<max_finger_size) && 
							(regions[k].height>min_finger_size) && (regions[k].height<max_finger_size) && diff < max_diff) {
							
							int dx = abs(regions[k].x - regions[j].x);
							int dy = abs(regions[k].y - regions[j].y);
							float dist = sqrt((float)(dx*dx+dy*dy));
							if (dist<=distance) { existing_finger=NULL; closest=dist; break;  }
						}
					}}
					
					if (existing_finger!=NULL) {
						for (std::list<FingerObject>::iterator test = fingerList.begin(); test!=fingerList.end(); test++) {
							FingerObject *test_finger = &(*test);
							if ( (test_finger!=existing_finger) && (test_finger->distance(regions[j].x,regions[j].y)<distance)) {
								//check if there is another fiducial with the same id closer
								existing_finger = NULL;
								break;
							}
						}}
					

				}
			}

			// update or add the finger
			if  (existing_finger!=NULL) {			
				existing_finger->update(regions[j].x,regions[j].y,regions[j].area);
			} else if (finger_match==1) {
				// add the newly found cursor
				FingerObject addFinger(width, height);				
				addFinger.update(regions[j].x,regions[j].y,regions[j].area);
				fingerList.push_back(addFinger);
			} else continue; //goto plain_analysis;
	
			drawObject(FINGER_ID,(int)(regions[j].x),(int)(regions[j].y),display,1);
			region_loop_end:;
		} /*else if (	(regions[j].width>average_leaf_size*2) && (regions[j].height>average_leaf_size*2)) {
			plain_analysis:;
			//do the plain object analysis larger than twice the leaf size.
	
			bool analyse = true;
			// check if object is within any found fiducial
			for (std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
				float distance = fiducial->distance(regions[j].x,regions[j].y);
				if (distance<average_fiducial_size/2) { analyse = false; break; }
			}
	
			if (analyse) obj_analyzer->process(&regions[j],src, dest, display);	

		}*/

	}

    if (tuio_server!=NULL) {
		sendTuioMessages();
		if (detect_finger) sendCursorMessages();
	} else if (midi_server!=NULL) sendMidiMessages();
	
	if (show_grid) drawGrid(src,dest,display);
	if (show_settings) drawGUI(display);
	//printStatistics(start_time);
    
    //long fiducial_time = SDLinterface::currentTime() - start_time;
    //std::cout << "fiducial latency: " << (fiducial_time/100.0f)/10.0f << "ms" << std::endl;
}

void FidtrackFinder::printStatistics(long start_time) {

	#ifdef WIN32
		long end_time = GetTickCount();
	#else
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv,&tz);
		long end_time = (tv.tv_sec*1000000)+(tv.tv_usec);
	#endif
		std::cout << "frame " << totalframes << " " << (end_time-start_time)/1000 << std::endl;
	
			for(std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {
				std::cout << totalframes << " " << fiducial->getStatistics() << std::endl;
			}
			for(std::list<FingerObject>::iterator finger = fingerList.begin(); finger!=fingerList.end(); finger++) {
				std::cout << totalframes << " " << finger->getStatistics() << std::endl;
			}
			
			

}

void FidtrackFinder::sendCursorMessages() {

	// get the current set of objects on the table
	// we do not only use the raw objects retrieved from the frame
	// but also the remaining (filtered) objects from our fiducialObject list
	bool sendAliveMessage = false;
	int aliveSize = 0;
	int *aliveList = new int[fingerList.size()];
		
	for(std::list<FingerObject>::iterator finger = fingerList.begin(); finger!=fingerList.end(); ) {

		int finger_status = finger->checkStatus(session_id);
		switch (finger_status) {
			case FINGER_ADDED: {
				aliveList[aliveSize]=finger->session_id;
				aliveSize++;
				session_id++;
				if(msg_listener) {
					std::stringstream add_message;
					add_message << "add cur " << finger->session_id;
					msg_listener->setMessage(add_message.str());
				}
				finger++;
				break;
			}
			case FINGER_REMOVED: {
				if(msg_listener) {
					std::stringstream del_message;
					del_message << "del cur " << finger->session_id;
					msg_listener->setMessage(del_message.str());
				}
				finger = fingerList.erase(finger);
				sendAliveMessage = true;
				break;
			}
			case FINGER_ALIVE: {
				aliveList[aliveSize]=finger->session_id;
				aliveSize++;
				finger++;
				break;
			}
			case FINGER_EXPIRED: {
				finger = fingerList.erase(finger);
				break;
			}
			case FINGER_CANDIDATE: {
				finger++;
				break;
			}
		}

	}

	
	long fseq = totalframes;
		
	// after a certain number of frames we should send the redundant alive message	
	if ((aliveSize>0) || (totalframes%30==0)) sendAliveMessage = true;
	if ((aliveSize==0) && (totalframes%30==0)) fseq = -1;
		
	// add the alive message
	if (sendAliveMessage) {
		tuio_server->addCurAlive(aliveList, aliveSize);
		
		char unsent = 0;
		// send a set message (in case anything changed compared to the last frame)
		for(std::list<FingerObject>::iterator finger = fingerList.begin(); finger!=fingerList.end(); finger++) {
			if (!tuio_server->freeCurSpace()) {
				tuio_server->addCurSeq(fseq);
				tuio_server->sendCurMessages();
			}
				
			std::string set_message = finger->addSetMessage(tuio_server);
			if(set_message!="" && msg_listener) msg_listener->setMessage(set_message);
		
			if (finger->unsent>unsent) unsent = finger->unsent;
		}
	
		// fill the rest of the packet with unchanged objects
		// sending the least sent ones first
		while (unsent>0) {
			for(std::list<FingerObject>::iterator finger = fingerList.begin(); finger!=fingerList.end(); finger++) {				
				if ((finger->alive) && (finger->unsent==unsent)) {
					if (tuio_server->freeCurSpace()) { 
						finger->redundantSetMessage(tuio_server);
					} else goto send;
				}
			}
			unsent--;
		}
	
		send:
		// finally send the packet
		tuio_server->addCurSeq(fseq);
		tuio_server->sendCurMessages();
	}
	delete[] aliveList;
}


	std::list<FiducialObject> FidtrackFinder::getCalibrationMarkers() {
		return  fiducialList;
	}
	
	
	std::list<FingerObject> FidtrackFinder::getCalibrationPoints() {
		return fingerList;	
	}

