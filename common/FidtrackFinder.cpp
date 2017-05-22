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
		std::cout << "target buffer must be grayscale" << std::endl;
		return false;
	}

	//help_text.push_back( "FidtrackFinder:");
	help_text.push_back( "   f - adjust finger size & sensitivity");
	help_text.push_back( "   b - adjust blob size & activation");
	help_text.push_back( "   y - toggle yamaarashi detection");

	initialize_treeidmap( &treeidmap, tree_config );	
	initialize_fidtrackerX( &fidtrackerx, &treeidmap, dmap);
	initialize_segmenter( &segmenter, width, height, treeidmap.max_adjacencies );
	BlobObject::setDimensions(width,height);

	//average_fiducial_size = height/2;
	average_fiducial_size = 0.0f;
	min_fiducial_size = max_fiducial_size = (int)average_fiducial_size;

	//position_threshold = 1.0f/(2*width); // half a pixel
	//rotation_threshold = M_PI/360.0f;	 // half a degree
	position_threshold = 1.0f/(width); // one pixel
	rotation_threshold = M_PI/180.0f;	 // one degree
	
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
	} else if (flag==KEY_Y) {
		detect_yamaarashi=!detect_yamaarashi;
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
					max_blob_size-=2;
					if (max_blob_size<min_blob_size) max_blob_size=0;
					if (max_blob_size<=0) {
						max_blob_size = 0;
						detect_blobs=false;
					}
				} else if (setObjectBlob) {
					send_fiducial_blobs = false;
				} else if (setFingerBlob) {
					send_finger_blobs = false;
				}
				break;
			case KEY_RIGHT:
				if (setBlobSize) {
					max_blob_size+=2;
					if ((min_blob_size>0) && (max_blob_size==2)) max_blob_size=min_blob_size;
					if (max_blob_size>height) max_blob_size=height;
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
		
		ui->drawText(17,14,"F          - exit finger configuration");
		// draw the finger
		if( average_finger_size>0) {
			ui->setColor(0,0,255);
			ui->drawEllipse(width/2, height/2, average_finger_size,average_finger_size);
		}
		
		sprintf(displayText,"finger size %d",average_finger_size);
		settingValue = average_finger_size;
		maxValue = 64;
	} else if (setFingerSensitivity) {
		ui->drawText(17,14,"F          - exit finger configuration");
		sprintf(displayText,"finger sensitivity %d",(int)floor(finger_sensitivity*100+0.5f));
		settingValue = (int)floor(finger_sensitivity*100+0.5f);
		maxValue = 200;
	} else if (setBlobSize) {
		
		ui->drawText(17,14,"B          - exit blob configuration");
		// draw the blob
		if( max_blob_size>0) {
			ui->setColor(0,0,255);
			ui->drawEllipse(width/2, height/2, max_blob_size,max_blob_size);
		}
			
		sprintf(displayText,"blob size %d",(int)max_blob_size);
		settingValue = max_blob_size;
		maxValue = height;
	} else if (setObjectBlob) {
		ui->drawText(17,14,"B          - exit blob configuration");
		sprintf(displayText,"fiducial blobs %d",send_fiducial_blobs);
		settingValue = send_fiducial_blobs;
		maxValue = 1;
	} else if (setFingerBlob) {
		ui->drawText(17,14,"B          - exit blob configuration");
		sprintf(displayText,"finger blobs %d",send_finger_blobs);
		settingValue = send_finger_blobs;
		maxValue = 1;
	} else return;
	
	ui->displayControl(displayText, 0, maxValue, settingValue);
}

void FidtrackFinder::decodeYamaarashi(FiducialX *yama, unsigned char *img, TuioTime ftime) {

	BlobObject *yamaBlob = NULL;
	try {
		yamaBlob = new BlobObject(ftime,yama->rootx, dmap);
	} catch (std::exception e) {
		yama->id = INVALID_FIDUCIAL_ID;
		return;
	}
	
	float bx = yamaBlob->getRawX();
	float by = yamaBlob->getRawY();
	
	float dx = (bx - yama->raw_x)/width;
	float dy = (by - yama->raw_y)/height;
	float dist = sqrtf(dx*dx + dy*dy);
	
	float bw = yamaBlob->getRawWidth()* 0.73f;
	float bh = yamaBlob->getRawHeight()* 0.73f;
	
	if (yamaBlob->getAngle()>1) {
		bw = bh;
		bh = yamaBlob->getRawWidth()* 0.73f;
	}
	
	float blob_area = M_PI * yamaBlob->getWidth()/2 * yamaBlob->getHeight()/2;
	float error = fabs(yamaBlob->getArea()/blob_area - 0.66f);

	delete yamaBlob;

	if ((error>0.1f) || (dist>0.01f)) {
		//std::cout << "yama fp: " << error << " " << dist << std::endl;
		yama->id = INVALID_FIDUCIAL_ID;
		return;
	}
	
	IntPoint point[4];
	int pixel;
	
	unsigned int value = 0;
	unsigned int checksum = 0;
	unsigned char bitpos = 0;
	
	float angle = yama->raw_a - M_PI_2;

	for (int i=0;i<6;i++) {
		
		float apos = angle;
		for (int p=0;p<4;p++) {
			
			point[p].x = (int)(bx + cosf(apos)*bw);
			point[p].y = (int)(by + sinf(apos)*bh);
			
			pixel = point[p].y*width+point[p].x;
			if ( (pixel<0) || (pixel>=width*height) ) {
				yama->id = INVALID_FIDUCIAL_ID;
				return;
			}
			
#ifndef NDEBUG
			ui->setColor(255, 0, 255);
			ui->drawLine(bx,by,point[p].x,point[p].y);
#endif
			
			if (bitpos<20) {
				if (img[pixel]==0) {
					unsigned int mask = (unsigned int)pow(2,bitpos);
					value = value|mask;
				}
			} else {
				if (img[pixel]==255) {
					unsigned int mask = (unsigned int)pow(2,bitpos-20);
					checksum = checksum|mask;
				}
			}
			
			if (invert_yamaarashi) apos-=M_PI_2;
			else apos+=M_PI_2;
			bitpos++;
		}
		
		if (invert_yamaarashi) angle -= M_PI/12.0f;
		else angle += M_PI/12.0f;
		
	}
	
	unsigned int validation = value%13;
	if (validation==checksum) yama->id = value;
	else {
		yama->id = FUZZY_FIDUCIAL_ID;
		//std::cout << "yama cs: " << value << " " << checksum << " " << validation << std::endl;
	}
	
	yama->raw_x = (yama->raw_x + bx)/2.0f;
	yama->raw_y = (yama->raw_y + by)/2.0f;
	
	if(!empty_grid) {
		int pixel = width*(int)floor(yama->raw_y+.5f) + (int)floor(yama->raw_x+.5f);
		if ((pixel>=0) || (pixel<width*height)) {
			yama->x = dmap[ pixel ].x/width;
			yama->y = dmap[ pixel ].y/height;
		}
	} else {
		yama->x = yama->raw_x/width;
		yama->y = yama->raw_y/height;
	}

}

float FidtrackFinder::checkFinger(BlobObject *fblob) {
	float blob_area = M_PI * fblob->getWidth()/2 * fblob->getHeight()/2;
	float confidence = fblob->getArea()/blob_area;
	if (confidence<0.6 || confidence>1.1) {
		return 1000.0f;
	}
	
	std::vector<BlobPoint> contourList = fblob->getFullContour();
	float bx = fblob->getRawX();
	float by = fblob->getRawY();
	float bw = fblob->getRawWidth()/2.0f;
	float bh = fblob->getRawHeight()/2.0f;
	float r = 2*M_PI-fblob->getAngle();
	float Sr = sinf(r);
	float Cr = cosf(r);
	
	float distance = 0.0f;
	for (unsigned int i = 0; i < contourList.size(); i++) {
		BlobPoint pt = contourList[i];
		float px = pt.x - bx;
		float py = pt.y - by;
		
		float pX = px*Cr - py*Sr;
		float pY = py*Cr + px*Sr;
		
		//ui->setColor(255,0,255);
		//ui->drawPoint(bx+pX,by+pY);
		
		float aE = atan2f(pY, pX);
		
		float eX = bw * cosf(aE);
		float eY = bh * sinf(aE);
		
		//ui->setColor(0,255,0);
		//ui->drawPoint(bx+eX,by+eY);
		
		float dx = pX-eX;
		float dy = pY-eY;
		
		distance += dx*dx+dy*dy;
	}

	float size = bw;
	if (bh > bw) size = bh;
	return (distance/contourList.size())/size;
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
	
	float min_object_size = min_fiducial_size / 1.2f;
	float max_object_size = max_fiducial_size * 1.2f;
	min_fiducial_size = max_fiducial_size = (int)average_fiducial_size;
	
	float min_finger_size = average_finger_size / 2.0f;
	float max_finger_size = average_finger_size * 2.0f;
	
	float min_region_size = min_finger_size;
	if ((average_finger_size==0) || (min_region_size>min_object_size)) min_region_size = min_object_size;
	else if ((min_blob_size>0) && (min_object_size>min_blob_size)) min_region_size = min_blob_size;
	
	float max_region_size = max_blob_size;
	if (max_blob_size<max_object_size) max_region_size = max_object_size;

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
		
		if( reg_count >= MAX_FIDUCIAL_COUNT ) continue;

		regions[reg_count].region = r;
		regions[reg_count].width = r->right-r->left+1;
		regions[reg_count].height = r->bottom-r->top+1;
		regions[reg_count].span = r->first_span;
		regions[reg_count].area = r->area;
		regions[reg_count].colour = r->colour;
		
		//if ((regions[reg_count].width>=min_region_size) && (regions[reg_count].width<=max_region_size) && (regions[reg_count].height>=min_region_size) && (regions[reg_count].height<=max_region_size)) {
		if ((regions[reg_count].width>3) && (regions[reg_count].width<height) && (regions[reg_count].height>3) && (regions[reg_count].height<height)) {
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
		}
	}

	float total_fiducial_size = 0.0f;
	int valid_fiducial_count = 0;
	// decode the found fiducials
	Region *next = fidtrackerx.root_regions_head.next;
	while( next != &fidtrackerx.root_regions_head ){
		
		compute_fiducial_statistics( &fidtrackerx, &fiducials[fid_count], next, width, height );
		
		if (fiducials[fid_count].id!=INVALID_FIDUCIAL_ID) {
			
			fiducials[fid_count].x = fiducials[fid_count].x/width;
			fiducials[fid_count].y = fiducials[fid_count].y/height;
			
			for( int i=0; i < reg_count; ++i ) {
				if (fiducials[fid_count].root==regions[i].region) {
					// assign the regionx data to the fiducial
					fiducials[fid_count].rootx = &regions[i];
					// we can also decode the yamaarashis now
					if (fiducials[fid_count].id==YAMA_ID) {
						if (detect_yamaarashi) decodeYamaarashi(&fiducials[fid_count], dest, frameTime);
						else fiducials[fid_count].id = INVALID_FIDUCIAL_ID;
					}
					break;
				}
			}
			
			if (fiducials[fid_count].id>max_fiducial_id)
				fiducials[fid_count].id = FUZZY_FIDUCIAL_ID;
			
			if (fiducials[fid_count].id>=0) {
				valid_fiducial_count ++;
				total_fiducial_size += fiducials[fid_count].root_size;
				if (fiducials[fid_count].root_size < min_fiducial_size) min_fiducial_size = fiducials[fid_count].root_size;
				if (fiducials[fid_count].root_size > max_fiducial_size) max_fiducial_size = fiducials[fid_count].root_size;
			} else if (fiducials[fid_count].id==YAMA_ID) fiducials[fid_count].id = INVALID_FIDUCIAL_ID;

			if (fiducials[fid_count].id!=INVALID_FIDUCIAL_ID) {
				fiducialList.push_back(&fiducials[fid_count]);
				fid_count ++;
			}
		}
		
		next = next->next;
		if( fid_count >= MAX_FIDUCIAL_COUNT ) break;
	}

	if (valid_fiducial_count>0) {
		average_fiducial_size = total_fiducial_size/valid_fiducial_count;
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
		
		bool add_blob = true;
		
		if ((regions[i].width>min_object_size) && (regions[i].width<max_object_size) &&
			(regions[i].height>min_object_size) && (regions[i].height<max_object_size) && diff < max_diff) {
			
			
			// ignore root blobs and inner blobs of found fiducials
			for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
				
				if ((*fid)->root==regions[i].region) {
					// ignore root blobs
					add_blob = false;
					break;
				} else {
					
					float dx = (*fid)->rootx->raw_x - regions[i].raw_x;
					float dy = (*fid)->rootx->raw_y - regions[i].raw_y;
					float distance = sqrtf(dx*dx+dy*dy);
					
					if (((regions[i].width+regions[i].height) < ((*fid)->rootx->width+(*fid)->rootx->height)) && (distance < average_fiducial_size/1.5f)) {
						// ignore inside blobs
						add_blob = false;
						break;
					}
					
				}
			}
			
			// ignore inner fiducial blobs
			/*if (add_blob) for( int j=0; j < reg_count; ++j ) {
				if (j==i) continue;
				
				float dx = regions[j].raw_x - regions[i].raw_x;
				float dy = regions[j].raw_y - regions[i].raw_y;
				float distance = sqrtf(dx*dx+dy*dy);

				if (((regions[i].width+regions[i].height) < (regions[j].width+regions[j].height)) && (distance < average_fiducial_size/1.5f)) {
					add_blob = false;
					break;
				}
			}*/
			
			// add the root regions
			if (add_blob) {
				BlobObject *root_blob = NULL;
				try {
					root_blob = new BlobObject(frameTime,&regions[i],dmap);
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
			//bool add_blob = true;
			for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
				
				int dx = regions[i].raw_x - (*fid)->raw_x;
				int dy = regions[i].raw_y - (*fid)->raw_y;
				float distance = sqrtf(dx*dx+dy*dy);
				
				if (distance < (*fid)->root_size/1.5f) {
					add_blob = false;
					break;
				}
			}
			
			// add the finger candidates
			if (add_blob) {
				BlobObject *finger_blob = NULL;
				try {
					finger_blob = new BlobObject(frameTime,&regions[i],dmap,true);
					fingerBlobs.push_back(finger_blob);
				} catch (std::exception e) { if (finger_blob) delete finger_blob; }
			}
			
		} else if (detect_blobs && (regions[i].colour==WHITE) && (regions[i].width>=min_blob_size) && (regions[i].height>=min_blob_size) && (regions[i].width<=max_blob_size) && (regions[i].height<=max_blob_size)) {
			
			// ignore saturated blobs
			if (regions[i].inner_span_count==16) continue;
			
			// add the remaining plain blob
			BlobObject *plain_blob = NULL;
			try {
				plain_blob = new BlobObject(frameTime,&regions[i],dmap);
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
	std::list<FiducialObject*> removeObjects;
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
			
			float da = existing_object->getAngleDiff(closest_fid->angle);
			float dp = width*existing_object->getDistance(closest_fid->x,closest_fid->y);
			
			existing_object->setFiducialInfo(closest_fid->root->colour,closest_fid->root_size);
			existing_object->setTrackingState(FIDUCIAL_FOUND);
			tuioManager->updateTuioObject(existing_object,closest_fid->x,closest_fid->y,closest_fid->angle);
			drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
			
			BlobObject *fid_blob = NULL;
			try {
				
				if ((da>M_PI/90.0f) || (dp>2)) {
					fid_blob = new BlobObject(frameTime,closest_fid->rootx,dmap);
					existing_object->setRootOffset(existing_object->getX()-fid_blob->getX(),existing_object->getY()-fid_blob->getY());
				}
				
				if (send_fiducial_blobs) {
					if (fid_blob==NULL) fid_blob = new BlobObject(frameTime,closest_fid->rootx,dmap);
					TuioBlob *existing_blob = tuioManager->getTuioBlob(existing_object->getSessionID());
					if (existing_blob) tuioManager->updateTuioBlob(existing_blob,fid_blob->getX(),fid_blob->getY(),fid_blob->getAngle(),fid_blob->getWidth(),fid_blob->getHeight(),fid_blob->getArea());
				}
				
			} catch (std::exception e) {}
			
			if (fid_blob!=NULL) delete fid_blob;
			fiducialList.remove(closest_fid);
		}
		// check for fuzzy ID
		else if ((alt_fid!=NULL) && (alt_fid->id==FUZZY_FIDUCIAL_ID) && (alt_closest<existing_object->getRootSize()/1.1f)) {
			
			 // don't update fuzzy position/angle for resting objects
			float distance = existing_object->getScreenDistance(alt_fid->x, alt_fid->y, width, height);
			if (distance<2) {
				alt_fid->x = existing_object->getX();
				alt_fid->y = existing_object->getY();
				alt_fid->angle = existing_object->getAngle();
			}
			
			existing_object->setTrackingState(FIDUCIAL_FUZZY);
			tuioManager->updateTuioObject(existing_object,alt_fid->x,alt_fid->y,alt_fid->angle);
			drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
			
			if (send_fiducial_blobs) {
				BlobObject *fid_blob = NULL;
				try {
					fid_blob = new BlobObject(frameTime,alt_fid->rootx,dmap);
					//existing_object->setRootOffset(existing_object->getX()-fid_blob->getX(),existing_object->getY()-fid_blob->getY());
					TuioBlob *existing_blob = tuioManager->getTuioBlob(existing_object->getSessionID());
					if (existing_blob) tuioManager->updateTuioBlob(existing_blob,fid_blob->getX(),fid_blob->getY(),fid_blob->getAngle(),fid_blob->getWidth(),fid_blob->getHeight(),fid_blob->getArea());
				} catch (std::exception e) {}
				if (fid_blob) delete fid_blob;
			}
			
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
				existing_object->setTrackingState(FIDUCIAL_FUZZY);
				
				tuioManager->updateTuioObject(existing_object,alt_fid->x,alt_fid->y,alt_fid->angle);
				drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
				
				BlobObject *fid_blob = NULL;
				try {
					fid_blob = new BlobObject(frameTime,alt_fid->rootx,dmap);
					existing_object->setRootOffset(existing_object->getX()-fid_blob->getX(),existing_object->getY()-fid_blob->getY());
					
					if (send_fiducial_blobs) {
						TuioBlob *existing_blob = tuioManager->getTuioBlob(existing_object->getSessionID());
						if (existing_blob) tuioManager->updateTuioBlob(existing_blob,fid_blob->getX(),fid_blob->getY(),fid_blob->getAngle(),fid_blob->getWidth(),fid_blob->getHeight(),fid_blob->getArea());
					}
				} catch (std::exception e) {}
				
				if (fid_blob) delete fid_blob;
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
				FloatPoint offset = existing_object->getRootOffset();
				existing_object->setTrackingState(FIDUCIAL_ROOT);
				
				float distance = existing_object->getScreenDistance(closest_rblob->getX(), closest_rblob->getY(), width, height);
				if (distance<2) {
					tuioManager->updateTuioObject(existing_object,closest_rblob->getX(),closest_rblob->getY(),existing_object->getAngle());
					drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
				} else {
					tuioManager->updateTuioObject(existing_object,closest_rblob->getX()+offset.x,closest_rblob->getY()+offset.y,existing_object->getAngle());
					drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
				}

				
				if (send_fiducial_blobs) {
					TuioBlob *existing_blob = tuioManager->getTuioBlob(existing_object->getSessionID());
					if (existing_blob) tuioManager->updateTuioBlob(existing_blob,closest_rblob->getX(),closest_rblob->getY(),closest_rblob->getAngle(),closest_rblob->getWidth(),closest_rblob->getHeight(),closest_rblob->getArea());
				}
				
				rootBlobs.remove(closest_rblob);
				delete closest_rblob;
				fiducialList.remove(closest_fid);
			}
		}
		
	}
	
	// remove corrected objects
	if (removeObjects.size()>0) {
		for (std::list<FiducialObject*>::iterator tobj = removeObjects.begin(); tobj!=removeObjects.end(); tobj++) {
			(*tobj)->setTrackingState(FIDUCIAL_LOST);
			tuioManager->removeTuioObject(*tobj);
		}
		objectList = tuioManager->getTuioObjects();
	}

	// -------------------------------------------------------------------------------------------------
	// add the remaining new fiducials
	for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
		
		FiducialX *fiducial = (*fid);
		if (fiducial->id<0) continue;
		
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

		if (objFilter) {
			add_object->addPositionFilter(5.0f,0.25f);
			//add_object->addAngleFilter(2.0f,0.25f);
		}
		tuioManager->addExternalTuioObject(add_object);
		drawObject(add_object->getSymbolID(),add_object->getX(),add_object->getY(),add_object->getTrackingState());
		
		BlobObject *fid_blob = NULL;
		try {
			fid_blob = new BlobObject(frameTime,fiducial->rootx,dmap);
			add_object->setRootOffset(add_object->getX()-fid_blob->getX(),add_object->getY()-fid_blob->getY());
			
			if (send_fiducial_blobs) {
				
				tuioManager->addExternalTuioBlob(fid_blob);
				fid_blob->setSessionID(add_object->getSessionID());
				
				fid_blob->addPositionThreshold(position_threshold*2.0f);
				fid_blob->addAngleThreshold(rotation_threshold*2.0f);
				fid_blob->addSizeThreshold(position_threshold*2.0f);
				
				if (blbFilter) {
					fid_blob->addPositionFilter(2.0f,0.25f);
					//fid_blob->addAngleFilter(0.5f,1.0f);
					fid_blob->addSizeFilter(5.0f,0.1f);
				}
			} else delete fid_blob;
			
		} catch (std::exception e) {}
		

	}
	
	std::list<TuioObject*> lostObjects = tuioManager->getUntouchedObjects();
	for(std::list<TuioObject*>::iterator iter = lostObjects.begin(); iter!=lostObjects.end(); iter++) {
		FiducialObject *tobj = (FiducialObject*)(*iter);
		if (tobj->getTrackingState()!=FIDUCIAL_LOST) {
			// update lost object with predicted position ... once only before removal
			TuioPoint fpos = tobj->predictPosition();
			tuioManager->updateTuioObject(tobj,fpos.getX(),fpos.getY(),tobj->getAngle());
			tobj->setTrackingState(FIDUCIAL_LOST);
		}
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

				if (send_finger_blobs) {
					TuioBlob *existing_blob = tuioManager->getTuioBlob((*tcur)->getSessionID());
					if (existing_blob) tuioManager->updateTuioBlob(existing_blob, closest_fblob->getX(),closest_fblob->getY(), closest_fblob->getAngle(), closest_fblob->getWidth(), closest_fblob->getHeight(), closest_fblob->getArea());
				}
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
			if (curFilter) add_cursor->addPositionFilter(5.0f,0.25f);
			drawObject(FINGER_ID,add_cursor->getX(),add_cursor->getY(),0);
			ui->setColor(0,255,0);
			ui->drawEllipse((*fblb)->getX()*width,(*fblb)->getY()*height,(*fblb)->getWidth()*width,(*fblb)->getHeight()*height,(*fblb)->getAngle());

			if (send_finger_blobs) {
				TuioBlob *cur_blob = tuioManager->addTuioBlob((*fblb)->getX(),(*fblb)->getY(),(*fblb)->getAngle(),(*fblb)->getWidth(),(*fblb)->getHeight(),(*fblb)->getArea());
				cur_blob->setSessionID(add_cursor->getSessionID());
				
				if (blbFilter) {
					cur_blob->addPositionFilter(2.0f,0.25f);
					//cur_blob->addAngleFilter(0.5f,1.0f);
					cur_blob->addSizeFilter(5.0f,0.1f);
				}
			}
			
		}
		
		delete (*fblb);
	}
}
	
if (detect_blobs) {
	
	// copy remaing "root blobs" into plain blob list
	for (std::list<BlobObject*>::iterator bobj = rootBlobs.begin(); bobj!=rootBlobs.end(); bobj++) {
		if (((*bobj)->getColour()==WHITE) && ((*bobj)->getRawWidth()>=min_blob_size)  && ((*bobj)->getRawHeight()>=min_blob_size)&& ((*bobj)->getRawWidth()<=max_blob_size)  && ((*bobj)->getRawHeight()<=max_blob_size)) plainBlobs.push_back((*bobj));
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
			
			if (blbFilter) {
				add_blob->addPositionFilter(2.0f,0.25f);
				//add_blob->addAngleFilter(0.5f,10.0f);
				add_blob->addSizeFilter(5.0f,0.1f);
			}
			tuioManager->addExternalTuioBlob(add_blob);
			drawObject(BLOB_ID,add_blob->getX(),add_blob->getY(),0);
			ui->setColor(0,0,255);
			ui->drawEllipse((*pblb)->getX()*width,(*pblb)->getY()*height,(*pblb)->getWidth()*width,(*pblb)->getHeight()*height,(*pblb)->getAngle());
			
			
		} else delete (*pblb);
	}
} else {
	// delete unused root blobs
	for (std::list<BlobObject*>::iterator rblb = rootBlobs.begin(); rblb!=rootBlobs.end(); rblb++) {
		delete (*rblb);
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

