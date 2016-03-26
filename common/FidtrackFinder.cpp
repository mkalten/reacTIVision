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

#include "FidtrackFinder.h"
#include <sstream>

using namespace TUIO;

bool FidtrackFinder::init(int w, int h, int sb, int db) {

	FiducialFinder::init(w,h,sb,db);

	if (db!=1) {
		printf("target buffer must be grayscale\n");
		return false;
	}

	help_text.push_back( "FidtrackFinder:");
	help_text.push_back( "   f - adjust finger size & sensitivity");

	initialize_treeidmap( &treeidmap, tree_config );	
	initialize_fidtrackerX( &fidtrackerx, &treeidmap, dmap);
	initialize_segmenter( &segmenter, width, height, treeidmap.max_adjacencies );
	BlobObject::setDimensions(width,height);

	average_fiducial_size = 48.0f;
	position_threshold = 1.0f/(2*width); // half a pixel
	rotation_threshold = M_PI/360.0f;	 // half a degree
	//position_threshold = 1.0f/(width); // one pixel
	//rotation_threshold = M_PI/180.0f;	 // one degree
	
	
	setFingerSize = false;
	setFingerSensitivity = false;
	
	setBlobSize = false;
	setObjectBlob = false;
	setFingerBlob = false;

	return true;
}

bool FidtrackFinder::toggleFlag(unsigned char flag, bool lock) {
	
	FiducialFinder::toggleFlag(flag,lock);
	
	if (flag==KEY_F) {
		if (setFingerSize || setFingerSensitivity) {
			setFingerSize = false;
			setFingerSensitivity = false;
			show_settings = false;
			return false;
		} else if(!lock) {
			setFingerSize = true;
			show_settings = true;
			return true;
		}
	} if (flag==KEY_B) {
		if (setBlobSize || setObjectBlob || setFingerBlob) {
			setBlobSize = false;
			setObjectBlob = false;
			setFingerBlob = false;
			show_settings = false;
			return false;
		} else if(!lock) {
			setBlobSize = true;
			show_settings = true;
			return true;
		}
	} else if (setFingerSize || setFingerSensitivity) {
		switch(flag) {
			case KEY_LEFT:
				if (setFingerSize) {
					average_finger_size--;
					if (average_finger_size<=1) average_finger_size=0;
					if (average_finger_size==0) detect_fingers=false;
				} else {
					finger_sensitivity-=0.05f;
					if (finger_sensitivity<0) finger_sensitivity=0;
				}
				break;
			case KEY_RIGHT:
				if (setFingerSize) {
					average_finger_size++;
					if (average_finger_size==1) average_finger_size=2;
					if (average_finger_size>64) average_finger_size=64;
					detect_fingers=true;
				} else {
					finger_sensitivity+=0.05f;
					if (finger_sensitivity>2) finger_sensitivity=2.0f;
				}
				break;
			case KEY_UP:
			case KEY_DOWN:
				if (setFingerSize) {
					setFingerSize=false;
					setFingerSensitivity=true;
				} else {
					setFingerSize=true;
					setFingerSensitivity=false;
				}
				break;
		}
	} else if (setBlobSize || setObjectBlob || setFingerBlob) {
		switch(flag) {
			case KEY_LEFT:
				if (setBlobSize) {
					max_blob_size--;
					if (max_blob_size<average_fiducial_size) max_blob_size=0;
					if (max_blob_size==0) detect_blobs=false;
				} else if (setObjectBlob) {
					send_fiducial_blobs = false;
				} else if (setFingerBlob) {
					send_finger_blobs = false;
				}
				break;
			case KEY_RIGHT:
				if (setBlobSize) {
					max_blob_size++;
					if (max_blob_size==1) max_blob_size=(int)average_fiducial_size;
					if (max_blob_size>height/2) max_blob_size=height/2;
					detect_blobs=true;
				} else if (setObjectBlob) {
					send_fiducial_blobs = true;
				} else if (setFingerBlob) {
					send_finger_blobs = true;
				}
				break;
			case KEY_UP:
				if (setFingerBlob) {
					setFingerBlob=false;
					setObjectBlob=true;
				} else if (setObjectBlob) {
					setObjectBlob=false;
					setBlobSize=true;
				} else if (setBlobSize) {
					setBlobSize=false;
					setFingerBlob=true;
				}
				break;
			case KEY_DOWN:
				if (setBlobSize) {
					setBlobSize=false;
					setObjectBlob=true;
				} else if (setObjectBlob) {
					setObjectBlob=false;
					setFingerBlob=true;
				} else if (setFingerBlob) {
					setFingerBlob=false;
					setBlobSize=true;
				}
				break;
		}
	}
	return lock;
}

void FidtrackFinder::displayControl() {
	
	FiducialFinder::displayControl();
	
	char displayText[64];
	int settingValue = 0;
	int maxValue = 0;
	
	if (setFingerSize) {
		
		// draw the finger
		if( average_finger_size>0) {
			ui->setColor(0,0,255);
			ui->drawEllipse(width/2, height/2, average_finger_size,average_finger_size);
		}
		
		sprintf(displayText,"finger size %d",average_finger_size);
		settingValue = average_finger_size;
		maxValue = 64;
	} else if (setFingerSensitivity) {
		sprintf(displayText,"finger sensitivity %d",(int)floor(finger_sensitivity*100+0.5f));
		settingValue = (int)floor(finger_sensitivity*100+0.5f);
		maxValue = 200;
	} else if (setBlobSize) {
			
		// draw the blob
		if( max_blob_size>0) {
			ui->setColor(0,0,255);
			ui->drawEllipse(width/2, height/2, max_blob_size,max_blob_size);
		}
			
		sprintf(displayText,"blob size %d",(int)max_blob_size);
		settingValue = max_blob_size;
		maxValue = height/2;
	} else if (setObjectBlob) {
		sprintf(displayText,"fiducial blobs %d",send_fiducial_blobs);
		settingValue = send_fiducial_blobs;
		maxValue = 1;
	} else if (setFingerBlob) {
		sprintf(displayText,"finger blobs %d",send_finger_blobs);
		settingValue = send_finger_blobs;
		maxValue = 1;
	} else return;
	
	ui->displayControl(displayText, 0, maxValue, settingValue);
}

void FidtrackFinder::decodeYamaarashi(FiducialX *yama, unsigned char *img) {

	
	BlobObject *yamaBlob;
	try {
		yamaBlob = new BlobObject(TuioTime::getSystemTime(),yama->rootx);
	} catch (std::exception e) {
		yama->id = FUZZY_FIDUCIAL_ID;
		return;
	}
	delete yamaBlob;
	
	double yx = yama->raw_x;
	double yy = yama->raw_y;
	double radius = yama->root_size * 0.75;
	double angle = yama->raw_a;
	
	IntPoint point[4];
	int pixel[4];
	
	unsigned int value = 0;
	unsigned int checksum = 0;
	unsigned char bitpos = 0;
	
	for (int i=0;i<6;i++) {
		double dx = sin(angle)*radius;
		double dy = cos(angle)*radius;
		
		point[0].x = (int)(yx + dx);
		point[0].y = (int)(yy - dy);
		
		point[1].x = (int)(yx - dx);
		point[1].y = (int)(yy + dy);
		
		point[2].x = (int)(yx + dy);
		point[2].y = (int)(yy + dx);
		
		point[3].x = (int)(yx - dy);
		point[3].y = (int)(yy - dx);
		
		for (int p=0;p<4;p++) {
			pixel[p] = point[p].y*width+point[p].x;
			if ( (pixel[p]<0) || (pixel[p]>=width*height) ) {
				yama->id = FUZZY_FIDUCIAL_ID;
				return;
			}
			
#ifndef NDEBUG
			ui->setColor(255, 0, 255);
			ui->drawLine(yx,yy,point[p].x,point[p].y);
#endif
			
			if (bitpos<20) {
				if (img[pixel[p]]==0) {
					unsigned int mask = (unsigned int)pow(2,bitpos);
					value = value|mask;
				}
			} else {
				if (img[pixel[p]]==255) {
					unsigned int mask = (unsigned int)pow(2,bitpos-20);
					checksum = checksum|mask;
				}
			}
			
			bitpos++;
		}
		
		angle += M_PI/12.0f;
	}
	
	unsigned int test = value%13;
	
	std::cout << value << " " << checksum << " " << test << std::endl;
	
	if (test==checksum) yama->id = value;
	else yama->id = FUZZY_FIDUCIAL_ID;
}

float FidtrackFinder::checkFinger(BlobObject *fblob) {
	float blob_area = M_PI * fblob->getWidth()/2 * fblob->getHeight()/2;
	float confidence = fblob->getArea()/blob_area;
	if (confidence<0.6 || confidence>1.1) {
		return 1000.0f;
	}
	
	std::vector<BlobPoint> contourList = fblob->getFullContour();
	float bx = fblob->getX()*width;
	float by = fblob->getY()*height;
	float r = 2*M_PI-fblob->getAngle();
	float Sr = sin(r);
	float Cr = cos(r);
	float distance = 0.0f;
	for (unsigned int i = 0; i < contourList.size(); i++) {
		BlobPoint pt = contourList[i];
		double px = pt.x - bx;
		double py = pt.y - by;
		
		double pX = px*Cr - py*Sr;
		double pY = py*Cr + px*Sr;
		
		//ui->setColor(255,0,255);
		//ui->drawPoint(bx+pX,by+pY);
		
		double aE = atan2(pY, pX);
		
		double eX = fblob->getWidth()*width/2.0f * cos(aE);
		double eY = fblob->getHeight()*height/2.0f * sin(aE);
		
		//ui->setColor(0,255,0);
		//ui->drawPoint(bx+eX,by+eY);
		
		double dx = pX-eX;
		double dy = pY-eY;
		
		distance += dx*dx+dy*dy;
	}

	if(!empty_grid) {
		int xp = (int)floor(fblob->getX()*width +0.5f);
		int yp = (int)floor(fblob->getY()*height+0.5f);
		int pixel = width*yp + xp;
		if ((pixel>=0) || (pixel<width*height)) {
			fblob->setX(dmap[ pixel ].x/width);
			fblob->setY(dmap[ pixel ].y/height);
		}
	}
	
	return (distance/contourList.size())/(fblob->getWidth()*width);
}

void FidtrackFinder::process(unsigned char *src, unsigned char *dest) {
	
	TuioTime frameTime = TuioTime::getSystemTime();
	tuioManager->initFrame(frameTime);
	//std::cout << "frame: " << totalframes << std::endl;
	
	std::list<TuioObject*> objectList = tuioManager->getTuioObjects();
	std::list<TuioCursor*> cursorList = tuioManager->getTuioCursors();
	std::list<TuioBlob*>   blobList   = tuioManager->getTuioBlobs();
	
	//std::cout << "tobjs: " << objectList.size() << std::endl;
	//std::cout << "fingers: " << fingerBlobs.size() << std::endl;
	//std::cout << "tblbs: " << blobList.size() << std::endl;

	std::list<FiducialX*> fiducialList;
	std::list<BlobObject*> fingerBlobs;
	std::list<BlobObject*> rootBlobs;
	std::list<BlobObject*> plainBlobs;
	
	float min_object_size = average_fiducial_size / 1.5f;
	float max_object_size = average_fiducial_size * 1.5f;
	
	float min_finger_size = average_finger_size / 2.0f;
	float max_finger_size = average_finger_size * 2.0f;
	
	float min_region_size = min_finger_size;
	float max_region_size = max_object_size*2;
	if ((average_finger_size==0) || (min_region_size>min_object_size)) min_region_size = min_object_size;
	
	float total_fiducial_size = 0.0f;
	int valid_fiducial_count = 0;
	int fid_count = 0;
	int reg_count = 0;
	
	//std::cout << "fiducial size: " << average_fiducial_size << std::endl;
	//std::cout << "finger size: " << average_finger_size << std::endl;
	
	// -----------------------------------------------------------------------------------------------
	// do the libfidtrack image segmentation
	step_segmenter( &segmenter, dest );

#ifndef NDEBUG
	sanity_check_region_initial_values( &segmenter );
#endif
	
	//int test_count = find_fiducialsX( fiducials, MAX_FIDUCIAL_COUNT, &fidtrackerx, &segmenter, width, height);
	//std::cout << "fidx: " << test_count << std::endl;
	
	// -----------------------------------------------------------------------------------------------
	// find the fiducial roots; starting at leafs
	initialize_head_region( &fidtrackerx.root_regions_head );
	for( int i=0; i < segmenter.region_count; ++i ) {
		Region *r = (Region*)(segmenter.regions + (segmenter.sizeof_region * (i)));

		if( r->adjacent_region_count == 1
		   && !(r->flags & (   SATURATED_REGION_FLAG |
							FRAGMENTED_REGION_FLAG |
							ADJACENT_TO_ROOT_REGION_FLAG |
							FREE_REGION_FLAG ) )
		   ) {

			assert( r->level == NOT_TRAVERSED );
			assert( r->children_visited_count == 0 );
			propagate_descendent_count_and_max_depth_upwards( &segmenter, r, &fidtrackerx);
		}
		
		regions[reg_count].region = r;
		regions[reg_count].width = r->right-r->left+1;
		regions[reg_count].height = r->bottom-r->top+1;
		regions[reg_count].span = r->first_span;
		regions[reg_count].area = r->area;
		regions[reg_count].colour = r->colour;
		
		if ((regions[reg_count].width>=min_region_size) && (regions[reg_count].width<=max_region_size) && (regions[reg_count].height>=min_region_size) && (regions[reg_count].height<=max_region_size)) {
			regions[reg_count].x = r->left+regions[reg_count].width/2.0f;
			regions[reg_count].y = r->top+regions[reg_count].height/2.0f;
			
			regions[reg_count].raw_x = regions[reg_count].x;
			regions[reg_count].raw_y = regions[reg_count].y;
			
			if(!empty_grid) {
				int pixel = width*(int)floor(regions[reg_count].y+.5f) + (int)floor(regions[reg_count].x+.5f);
				if ((pixel>=0) || (pixel<width*height)) {
					regions[reg_count].x = dmap[ pixel ].x;
					regions[reg_count].y = dmap[ pixel ].y;
				}
				//if ((regions[reg_count].x==0) && (regions[reg_count].y==0)) continue;
			}
			
			regions[reg_count].x = regions[reg_count].x/width;
			regions[reg_count].y = regions[reg_count].y/height;
			
			regions[reg_count].inner_span_count = 0;
			for (int s=0; s<r->adjacent_region_count;s++) {
				if ((r->adjacent_regions[s]->right-r->adjacent_regions[s]->left)<(r->right-r->left)) {
					regions[reg_count].inner_spans[regions[reg_count].inner_span_count] = r->adjacent_regions[s]->first_span;
					regions[reg_count].inner_span_count++;
					if (regions[reg_count].inner_span_count==16) break;
				}
			}
			
			++reg_count;
			if( reg_count >= MAX_FIDUCIAL_COUNT ) break;

		}
	}

	
	// decode the found fiducials
	Region *next = fidtrackerx.root_regions_head.next;
	while( next != &fidtrackerx.root_regions_head ){
		
		compute_fiducial_statistics( &fidtrackerx, &fiducials[fid_count], next, width, height );
		
		if (fiducials[fid_count].id!=INVALID_FIDUCIAL_ID) {
			
			fiducials[fid_count].x = fiducials[fid_count].x/width;
			fiducials[fid_count].y = fiducials[fid_count].y/height;
			
			if (fiducials[fid_count].id>=0) {
				valid_fiducial_count ++;
				total_fiducial_size += fiducials[fid_count].root_size;
			}

			fiducials[fid_count].rootx = NULL;
			fiducialList.push_back(&fiducials[fid_count]);
		}
		
		next = next->next;
		++fid_count;
		if( fid_count >= MAX_FIDUCIAL_COUNT ) break;
	}

	
	if (valid_fiducial_count>0) {
		average_fiducial_size = (average_fiducial_size+(total_fiducial_size/(float)valid_fiducial_count))/2.0f;
	}

	//std::cout << "fiducials: " << fid_count << std::endl;
	//std::cout << "regions: " << reg_count << std::endl;
	
	// -----------------------------------------------------------------------------------------------
	// assign the plain regions
	for( int i=0; i < reg_count; ++i ) {
		
		int diff = abs(regions[i].width - regions[i].height);
		int max_diff = regions[i].width;
		if (regions[i].height > regions[i].width)
			max_diff = regions[i].height;
		
		if ((regions[i].width>min_object_size) && (regions[i].width<max_object_size) &&
			(regions[i].height>min_object_size) && (regions[i].height<max_object_size) && diff < max_diff) {
			
			// ignore root blobs of found fiducials
			bool add_blob = true;
			for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
				
				float dx = regions[i].raw_x - (*fid)->raw_x;
				float dy = regions[i].raw_y - (*fid)->raw_y;
				float distance = sqrtf(dx*dx+dy*dy);
								
				if (distance < (*fid)->root_size/4.0f) {
					
					if ((*fid)->root==regions[i].region) {
						// assig the regionx data to the fiducial
						(*fid)->rootx = &regions[i];
						// we can also decode the yamaarashis now
						if ((detect_yamaarashi) && ((*fid)->id==YAMA_ID)) decodeYamaarashi((*fid), dest);
					}

					add_blob = false;
					break;
				}
			}
			
			// ignore inner fiducial blobs
			for( int j=0; j < reg_count; ++j ) {
				if (j==i) continue;
				
				float dx = regions[j].raw_x - regions[i].raw_x;
				float dy = regions[j].raw_y - regions[i].raw_y;
				float distance = sqrtf(dx*dx+dy*dy);
				
				if (((regions[i].width+regions[i].height) < (regions[j].width+regions[j].height)) && (distance < average_fiducial_size/4.0f)) {
					add_blob = false;
					break;
				}
			}
			
			// add the root regions
			if (add_blob) {
				BlobObject *root_blob = NULL;
				try {
					root_blob = new BlobObject(frameTime,&regions[i]);
					rootBlobs.push_back(root_blob);
				} catch (std::exception e) { if (root_blob) delete root_blob; }
			}
			
		} else if (detect_fingers && (regions[i].colour==WHITE) && (regions[i].width>min_finger_size) && (regions[i].width<max_finger_size) &&
				   (regions[i].height>min_finger_size) && (regions[i].height<max_finger_size) && diff < max_diff) {
			
			// ignore noisy blobs
			if (regions[i].inner_span_count>3*finger_sensitivity) {
				//std::cout << "inner spans: "<< regions[j].inner_span_count << std::endl;
				continue;
			}
			
			// ignore fingers within and near current fiducials
			bool add_blob = true;
			for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
				
				int dx = regions[i].raw_x - (*fid)->raw_x;
				int dy = regions[i].raw_y - (*fid)->raw_y;
				float distance = sqrtf(dx*dx+dy*dy);
				
				if (distance < (*fid)->root_size/1.4f) {
					add_blob = false;
					break;
				}
			}
			
			// add the finger candidates
			if (add_blob) {
				BlobObject *finger_blob = NULL;
				try {
					finger_blob = new BlobObject(frameTime,&regions[i],true);
					fingerBlobs.push_back(finger_blob);
				} catch (std::exception e) { if (finger_blob) delete finger_blob; }
			}
			
		} else if (detect_blobs && (regions[i].colour==WHITE) && (regions[i].width>=max_object_size) && (regions[i].height>=max_object_size)) {
			
			// ignore saturated blobs
			if (regions[i].inner_span_count==16) continue;
			
			// add the remaining plain blob
			BlobObject *plain_blob = NULL;
			try {
				plain_blob = new BlobObject(frameTime,&regions[i]);
				plainBlobs.push_back(plain_blob);
			} catch (std::exception e) { if (plain_blob) delete plain_blob; }
		}
		
	}
	
	//std::cout << "roots: " << rootBlobs.size() << std::endl;
	//std::cout << "fingers: " << fingerBlobs.size() << std::endl;
	//std::cout << "blobs: " << plainBlobs.size() << std::endl;
	
/*
	float dx = fiducials[i].x - regions[j].x;
	float dy = fiducials[i].y - regions[j].y;
	distance = sqrt(dx*dx+dy*dy);
	if ( distance<average_fiducial_size/1.5f) {
		existing_fiducial->setBlobOffset(dx,dy);
	}
	
	FloatPoint offset = existing_fiducial->getBlobOffset();
	float xpos = regions[j].x + offset.x;
	float ypos = regions[j].y + offset.y;
*/
	
	// -----------------------------------------------------------------------------------------------
	// update existing fiducials
	std::list<TuioObject*> removeObjects;
	for (std::list<TuioObject*>::iterator tobj = objectList.begin(); tobj!=objectList.end(); tobj++) {
		//ui->setColor(255,255,0);
		//ui->fillEllipse((*tobj)->getX()*width,(*tobj)->getY()*height,5,5);
		FiducialObject *existing_object = (FiducialObject*)(*tobj);
		TuioPoint fpos = existing_object->predictPosition();
		//ui->setColor(255,0,0);
		//ui->fillEllipse(fpos.getX()*width,fpos.getY()*height,10,10);
		
		float closest = width;
		float alt_closest = width;
		FiducialX *closest_fid = NULL;
		FiducialX *alt_fid = NULL;
		
		for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
			FiducialX *fiducial = (*fid);
			if (fiducial->id==INVALID_FIDUCIAL_ID) continue;
			
			float distance = fpos.getScreenDistance(fiducial->x,fiducial->y,width,height);
			
			if ((fiducial->id==(*tobj)->getSymbolID()) && (distance<closest)) {
				closest_fid = fiducial;
				closest = distance;
			} else if ((fiducial->id!=(*tobj)->getSymbolID()) && (distance<alt_closest)) {
				// save closest alternative with wrong or fuzzy ID
				alt_fid = fiducial;
				alt_closest = distance;
			}
		}
		
		// check if another object claims this fiducial id
		if (closest_fid!=NULL) {
			
			for (std::list<TuioObject*>::iterator fobj = objectList.begin(); fobj!=objectList.end(); fobj++) {
				if ((*fobj)==(*tobj)) continue;
				TuioPoint opos = (*fobj)->predictPosition();
				float distance = opos.getDistance(closest_fid->x,closest_fid->y);
				if ((closest_fid->id==(*fobj)->getSymbolID()) && (distance<closest)) {
					closest_fid = NULL;
					break;
				}
			}
		}
		
		// we found an existing fiducial object
		if (closest_fid!=NULL) {
			
			//existing_object->setFiducialInfo(closest_fid->root_colour,closest_fid->root_size);
			existing_object->setTrackingState(FIDUCIAL_FOUND);
			tuioManager->updateTuioObject(existing_object,closest_fid->x,closest_fid->y,closest_fid->angle);
			drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
			
			fiducialList.remove(closest_fid);
		}
		// check for fuzzy ID
		else if ((alt_fid!=NULL) && (alt_fid->id==FUZZY_FIDUCIAL_ID) && (alt_closest<existing_object->getRootSize()/1.1f)) {
			
			existing_object->setTrackingState(FIDUCIAL_FUZZY);
			tuioManager->updateTuioObject(existing_object,alt_fid->x,alt_fid->y,existing_object->getAngle());
			
			drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
			
			fiducialList.remove(alt_fid);
		}
		// possibly correct a wrong ID
		else if ((alt_fid!=NULL) && (alt_closest<existing_object->getRootSize()/1.1f)) {
			
			if (existing_object->checkIdConflict(alt_fid->id)) {
				removeObjects.push_back(existing_object);
				if (tuioManager->isVerbose()) printf("removed wrong ID %d (%ld)\n", existing_object->getSymbolID(),existing_object->getSessionID());
			} else {
				if (tuioManager->isVerbose()) printf("corrected wrong ID from %d to %d (%ld)\n", alt_fid->id,existing_object->getSymbolID(),existing_object->getSessionID());
				alt_fid->id=existing_object->getSymbolID();
				existing_object->setTrackingState(FIDUCIAL_FOUND);
				
				tuioManager->updateTuioObject(existing_object,alt_fid->x,alt_fid->y,alt_fid->angle);
				drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
				
				fiducialList.remove(alt_fid);
			}
			
			
		}
		// root region tracking
		else {
			
			float closest = width;
			BlobObject *closest_rblob = NULL;
			for (std::list<BlobObject*>::iterator rblb = rootBlobs.begin(); rblb!=rootBlobs.end(); rblb++) {
				BlobObject *root_blob = (*rblb);
				float distance = fpos.getScreenDistance(root_blob->getX(),root_blob->getY(),width,height);
				if ((distance<closest) && (distance<existing_object->getRootSize()/1.1f) && (root_blob->getColour()==existing_object->getRootColour())) {
					closest_rblob = root_blob;
					closest = distance;
				}
			}
			
			// we found a nearby root blob
			if (closest_rblob!=NULL) {
				existing_object->setTrackingState(FIDUCIAL_ROOT);
				tuioManager->updateTuioObject(existing_object,closest_rblob->getX(),closest_rblob->getY(),existing_object->getAngle());
 				drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
				
				rootBlobs.remove(closest_rblob);
				delete closest_rblob;
				fiducialList.remove(closest_fid);
			}
		}
		
	}
	
	// remove corrected objects
	if (removeObjects.size()>0) {
		for (std::list<TuioObject*>::iterator tobj = removeObjects.begin(); tobj!=removeObjects.end(); tobj++) {
			tuioManager->removeTuioObject(*tobj);
		}
		objectList = tuioManager->getTuioObjects();
	}

	// -------------------------------------------------------------------------------------------------
	// add the remaining new fiducials
	for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
		
		FiducialX *fiducial = (*fid);
		if (fiducial->id==FUZZY_FIDUCIAL_ID) continue;
		
		// remove possible blobs under this fiducial
		bool update_blob_list = false;
		for (std::list<TuioBlob*>::iterator tblb = blobList.begin(); tblb!=blobList.end(); tblb++) {
			TuioPoint bpos = (*tblb)->predictPosition();
			float distance = bpos.getScreenDistance(fiducial->x, fiducial->y,width,height);
			if (distance<fiducial->root_size/4.0f) {
				tuioManager->removeTuioBlob((*tblb));
				update_blob_list = true;
				//break;
			}
		}
		if (update_blob_list) blobList = tuioManager->getTuioBlobs();

		// remove possible fingers around this position
		bool update_cursor_list = false;
		for (std::list<TuioCursor*>::iterator tcur = cursorList.begin(); tcur!=cursorList.end(); tcur++) {
			TuioPoint bpos = (*tcur)->predictPosition();
			float distance = bpos.getScreenDistance(fiducial->x, fiducial->y,width,height);
			if (distance<fiducial->root_size/1.4f) {
				tuioManager->removeTuioCursor((*tcur));
				update_cursor_list = true;
			}
		}
		if (update_cursor_list) cursorList = tuioManager->getTuioCursors();
		
		FiducialObject *add_object = new FiducialObject(frameTime,0,fiducial->id,fiducial->x,fiducial->y,fiducial->angle);
		
		add_object->setFiducialInfo(fiducial->root->colour,fiducial->root_size);
		add_object->addPositionThreshold(position_threshold);
		add_object->addAngleThreshold(rotation_threshold);

		add_object->addPositionFilter(5.0f,10.0f);
		//add_object->addAngleFilter(0.5f,10.0f);
		tuioManager->addExternalTuioObject(add_object);
		drawObject(add_object->getSymbolID(),add_object->getX(),add_object->getY(),add_object->getTrackingState());
	}
	
	std::list<TuioObject*> lostObjects = tuioManager->getUntouchedObjects();
	for(std::list<TuioObject*>::iterator iter = lostObjects.begin(); iter!=lostObjects.end(); iter++) {
		FiducialObject *tobj = (FiducialObject*)(*iter);
		tobj->setTrackingState(FIDUCIAL_LOST);
	}

 if (detect_fingers) {
	// -----------------------------------------------------------------------------------------------
	// update existing fingers
	for (std::list<TuioCursor*>::iterator tcur = cursorList.begin(); tcur!=cursorList.end(); tcur++) {
		//ui->setColor(255,255,0);
		//ui->fillEllipse((*tcur)->getX()*width,(*tcur)->getY()*height,10,10);
		TuioPoint cpos = (*tcur)->predictPosition();
		//ui->setColor(255,0,0);
		//ui->fillEllipse(cpos.getX()*width,cpos.getY()*height,10,10);
		
		float closest = width;
		BlobObject *closest_fblob = NULL;
		for (std::list<BlobObject*>::iterator fblb = fingerBlobs.begin(); fblb!=fingerBlobs.end(); fblb++) {
			float distance = (*fblb)->getScreenDistance(cpos.getX(),cpos.getY(),width,height);
			
			if ((distance<average_finger_size*2.0f) && (distance<=closest)) {
				closest_fblob = *fblb;
				closest = distance;
			}
		}
		
		// check if another cursor claims this finger blob
		if (closest_fblob!=NULL) {
			
			for (std::list<TuioCursor*>::iterator ocur = cursorList.begin(); ocur!=cursorList.end(); ocur++) {
				if ((*ocur)==(*tcur)) continue;
 				TuioPoint opos = (*ocur)->predictPosition();
				float distance = closest_fblob->getScreenDistance(cpos.getX(),cpos.getY(),width,height);
				if (distance<closest) {
					closest_fblob = NULL;
					break;
				}
			}
		}
		
		// we found an existing finger blob
		if (closest_fblob!=NULL) {
			
			float finger_match = checkFinger(closest_fblob);
			float adaptive_sensitivity = finger_sensitivity+(*tcur)->getMotionSpeed();
			if ((*tcur)->getTuioState()==TUIO_ADDED) adaptive_sensitivity = adaptive_sensitivity/3.0f;
			
			if(finger_match<adaptive_sensitivity) {
			
				tuioManager->updateTuioCursor((*tcur),closest_fblob->getX(),closest_fblob->getY());
				drawObject(FINGER_ID,(*tcur)->getX(),(*tcur)->getY(),0);
				ui->setColor(0,255,0);
				ui->drawEllipse(closest_fblob->getX()*width,closest_fblob->getY()*height,closest_fblob->getWidth()*width,closest_fblob->getHeight()*height,closest_fblob->getAngle());

				//TuioBlob *existing_blob = tuioManager->getTuioBlob((*tcur)->getSessionID());
				//tuioManager->updateTuioBlob(existing_blob, existing_fblob->getX(),existing_fblob->getY(), existing_fblob->getAngle(), existing_fblob->getWidth(), existing_fblob->getHeight(), existing_fblob->getArea());
			}
			
			fingerBlobs.remove(closest_fblob);
			delete closest_fblob;
		}
		// check for fingers in the predicted region
		else {
			//printf("where oh where\n");
			
		}
	}

	// -------------------------------------------------------------------------------------------------
	// add the remaining new fingers
	for (std::list<BlobObject*>::iterator fblb = fingerBlobs.begin(); fblb!=fingerBlobs.end(); fblb++) {
		
		float finger_match = checkFinger(*fblb);
		if(finger_match<finger_sensitivity/4.0f) {
			TuioCursor *add_cursor = tuioManager->addTuioCursor((*fblb)->getX(),(*fblb)->getY());
			add_cursor->addPositionThreshold(position_threshold*2.0f); //1px
			add_cursor->addPositionFilter(5.0f,10.0f);
			drawObject(FINGER_ID,add_cursor->getX(),add_cursor->getY(),0);
			ui->setColor(0,255,0);
			ui->drawEllipse((*fblb)->getX()*width,(*fblb)->getY()*height,(*fblb)->getWidth()*width,(*fblb)->getHeight()*height,(*fblb)->getAngle());

			//TuioBlob *add_blob = tuioManager->addTuioBlob(finger_blob->getX(),finger_blob->getY(),finger_blob->getAngle(),finger_blob->getWidth(),finger_blob->getHeight(),finger_blob->getArea());
			//add_blob->setSessionID(add_cursor->getSessionID());
			
		}
		
		delete (*fblb);
	}
}
	
if (detect_blobs) {
	
	// copy remaing "root blobs" into plain blob list
	for (std::list<BlobObject*>::iterator bobj = rootBlobs.begin(); bobj!=rootBlobs.end(); bobj++) {
		if ((*bobj)->getColour()==WHITE) plainBlobs.push_back((*bobj));
		else delete (*bobj);
	}
	
	// -----------------------------------------------------------------------------------------------
	// update existing blobs
	for (std::list<TuioBlob*>::iterator tblb = blobList.begin(); tblb!=blobList.end(); tblb++) {
		//ui->setColor(255,255,0);
		//ui->fillEllipse((*tcur)->getX()*width,(*tcur)->getY()*height,10,10);
		TuioPoint bpos = (*tblb)->predictPosition();
		//ui->setColor(255,0,0);
		//ui->fillEllipse(cpos.getX()*width,cpos.getY()*height,10,10);
		
		float closest = width;
		BlobObject *closest_blob = NULL;
		for (std::list<BlobObject*>::iterator pblb = plainBlobs.begin(); pblb!=plainBlobs.end(); pblb++) {
			float distance = (*pblb)->getDistance(bpos.getX(),bpos.getY());
			
			if ((distance<(*tblb)->getWidth()) && (distance<=closest)) {
				closest_blob = *pblb;
				closest = distance;
			}
		}
		
		// check if another blob claims this one
		if (closest_blob!=NULL) {
			
			for (std::list<TuioBlob*>::iterator oblb = blobList.begin(); oblb!=blobList.end(); oblb++) {
				if ((*oblb)==(*tblb)) continue;
				TuioPoint opos = (*oblb)->predictPosition();
				float distance = closest_blob->getDistance(bpos.getX(),bpos.getY());
				if (distance<closest) {
					closest_blob = NULL;
					break;
				}
			}
		}
		
		// we found an existing blob
		if (closest_blob!=NULL) {

			tuioManager->updateTuioBlob((*tblb),closest_blob->getX(),closest_blob->getY(),closest_blob->getAngle(),closest_blob->getWidth(),closest_blob->getHeight(),closest_blob->getArea());
			drawObject(BLOB_ID,(*tblb)->getX(),(*tblb)->getY(),0);
			
			ui->setColor(0,0,255);
			ui->drawEllipse((*tblb)->getX()*width,(*tblb)->getY()*height,(*tblb)->getWidth()*width,(*tblb)->getHeight()*height,(*tblb)->getAngle());
			
			plainBlobs.remove(closest_blob);
			delete closest_blob;
		}
	}
	
	// -------------------------------------------------------------------------------------------------
	// add the remaining new blobs
	for (std::list<BlobObject*>::iterator pblb = plainBlobs.begin(); pblb!=plainBlobs.end(); pblb++) {
		
		// double check if blob is within existing fiducial
		bool add_blob = true;
		objectList = tuioManager->getTuioObjects();
		for (std::list<TuioObject*>::iterator iter = objectList.begin(); iter!=objectList.end(); iter++) {
			FiducialObject *tobj = (FiducialObject*)(*iter);
			float distance = tobj->getScreenDistance((*pblb)->getX(), (*pblb)->getY(),width,height);
			if (distance<tobj->getRootSize()/2.0f) {
				add_blob = false;
				break;
			}
		}
		if (add_blob) {

			TuioBlob *add_blob = (*pblb);
			//TuioBlob *add_blob = tuioManager->addTuioBlob((*pblb)->getX(),(*pblb)->getY(),(*pblb)->getAngle(),(*pblb)->getWidth(),(*pblb)->getHeight(),(*pblb)->getArea());
			add_blob->addPositionThreshold(position_threshold*2.0f);
			add_blob->addAngleThreshold(rotation_threshold*2.0f);
			add_blob->addSizeThreshold(position_threshold*2.0f);
			
			add_blob->addPositionFilter(2.0f,10.0f);
			//add_blob->addAngleFilter(0.5f,10.0f);
			add_blob->addSizeFilter(10.0f,1.0f);
			tuioManager->addExternalTuioBlob(add_blob);
			drawObject(BLOB_ID,add_blob->getX(),add_blob->getY(),0);
			ui->setColor(0,0,255);
			ui->drawEllipse((*pblb)->getX()*width,(*pblb)->getY()*height,(*pblb)->getWidth()*width,(*pblb)->getHeight()*height,(*pblb)->getAngle());
			
			
		} else delete (*pblb);
	}
}
	tuioManager->stopUntouchedMovingObjects();
	tuioManager->stopUntouchedMovingCursors();
	tuioManager->stopUntouchedMovingBlobs();
	tuioManager->removeUntouchedStoppedObjects();
	tuioManager->removeUntouchedStoppedCursors();
	tuioManager->removeUntouchedStoppedBlobs();
	//printStatistics(frameTime);
	((TuioServer*)tuioManager)->commitFrame();
	
	if (show_grid) drawGrid(src,dest);
	if (show_settings) displayControl();
	totalframes++;
}

void FidtrackFinder::printStatistics(TuioTime frameTime) {
	
	TuioTime endTime = TuioTime::getSystemTime() -frameTime;
	
	std::list<TuioObject*> objectList = tuioManager->getTuioObjects();
	std::list<TuioCursor*> cursorList = tuioManager->getTuioCursors();
	std::list<TuioBlob*> blobList = tuioManager->getTuioBlobs();
	
	std::cout << "fseq " << totalframes << " " << endTime.getMicroseconds()/1000.0f << std::endl;
	//if (objectList.size()>0) std::cout<< objectList.size() << std::endl;
	//else std::cout<< "NONE" << std::endl;
	
	for(std::list<TuioObject*>::iterator iter = objectList.begin(); iter!=objectList.end(); iter++) {
		FiducialObject *tobj = (FiducialObject*)(*iter);
		std::cout << tobj->getStatistics() << std::endl;
	}
	/*for(std::list<TuioCursor*>::iterator iter = cursorList.begin(); iter!=cursorList.end(); iter++) {
		TuioCursor *tcur =  (*iter);
		std::cout << "cur FND " << tcur->getSessionID() << " " << tcur->getCursorID() << " " << tcur->getX() << " " << tcur->getY() << std::endl;
	}*/
	
	std::cout << std::endl;
}

