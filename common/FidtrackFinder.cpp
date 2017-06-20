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
	
	get_black_roots = false;
	get_white_roots = false;

	//position_threshold = 1.0f/(2*width); // half a pixel
	//rotation_threshold = M_PI/360.0f;	 // half a degree
	position_threshold = 1.0f/(width); // one pixel
	rotation_threshold = M_PI/180.0f;	 // one degree
	
	setYamarashi = false;
	setYamaFlip = false;
	
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
	} else if (flag==KEY_B) {
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
		if (setYamarashi || setYamaFlip) {
			setYamarashi = false;
			setYamaFlip = false;
			show_settings = false;
			return false;
		} else if(!lock) {
			setYamarashi = true;
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
	} else if (setYamarashi || setYamaFlip) {
		switch(flag) {
			case KEY_LEFT:
				if (setYamarashi) detect_yamaarashi = false;
				else invert_yamaarashi = false;
				break;
			case KEY_RIGHT:
				if (setYamarashi) detect_yamaarashi = true;
				else invert_yamaarashi = true;
				break;
			case KEY_UP:
			case KEY_DOWN:
				if (setYamarashi) {
					setYamarashi=false;
					setYamaFlip=true;
				} else {
					setYamaFlip=false;
					setYamarashi=true;
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
	} else if (setYamarashi) {
		
		ui->drawText(17,14,"Y          - exit yamaarashi configuration");
		sprintf(displayText,"enable yamaarashi %d",detect_yamaarashi);
		settingValue = detect_yamaarashi;
		maxValue = 1;
	} else if (setYamaFlip) {
		
		ui->drawText(17,14,"Y          - exit yamaarashi configuration");
		sprintf(displayText,"invert yamaarashi %d",invert_yamaarashi);
		settingValue = invert_yamaarashi;
		maxValue = 1;
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

int orientation(BlobPoint *p, BlobPoint *q, BlobPoint *r)
{
	int val = (q->y - p->y) * (r->x - q->x) - (q->x - p->x) * (r->y - q->y);
	if (val == 0) return 0;  // colinear
 
	return (val > 0)? 1: 2;  // clock or counterclock wise
}

bool intersection(BlobPoint *p1, BlobPoint *q1, BlobPoint *p2, BlobPoint *q2)
{
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);
	
	if (o1 != o2 && o3 != o4) return true;
	else return false;
}

void FidtrackFinder::decodeYamaarashi(FiducialX *yama, unsigned char *img, TuioTime ftime) {


	BlobObject *yamaBlob = NULL;
	try {
		yamaBlob = new BlobObject(ftime,yama->root, dmap);
	} catch (std::exception e) {
		yama->id = INVALID_FIDUCIAL_ID;
		return;
	}
	
	float bx = yamaBlob->getRawX();
	float by = yamaBlob->getRawY();
	
	float dx = (bx - yama->raw_x)/width;
	float dy = (by - yama->raw_y)/height;
	float dist = sqrtf(dx*dx + dy*dy);
	
	float bw = yamaBlob->getRawWidth()/2.0f;
	float bh = yamaBlob->getRawHeight()/2.0f;
	float ba = yamaBlob->getAngle();

	//ui->setColor(0, 255, 0);
	//ui->drawEllipse(bx,by,yamaBlob->getRawWidth(),yamaBlob->getRawHeight(),ba);
	
	float blob_area = M_PI * yamaBlob->getWidth()/2 * yamaBlob->getHeight()/2;
	float error = fabs(yamaBlob->getArea()/blob_area - 0.66f);

	delete yamaBlob;

	if ((error>0.1f) || (dist>0.01f)) {
		//std::cout << "yama fp: " << error << " " << dist << std::endl;
		yama->id = INVALID_FIDUCIAL_ID;
		return;
	}
	
	// get black and white leaf nodes
	int ib = 0, iw = 0;
	BlobPoint black_leaf[2], white_leaf[2];
	for (int i=0; i<yama->root->adjacent_region_count; i++) {
		Region *black_region = yama->root->adjacent_regions[i];
		if (black_region->size < yama->root->size) {
			if (black_region->adjacent_region_count==1) {
				black_leaf[ib].set(black_region->raw_x, black_region->raw_y);
				ib++;
			} else {
				for (int j=0; j<black_region->adjacent_region_count; j++) {
					Region *white_region = black_region->adjacent_regions[j];
					if (white_region->size < black_region->size) {
						white_leaf[iw].set(white_region->raw_x, white_region->raw_y);
						iw++;
					}
				}
			}
		}
	}
	
	// correct intersection
	if (intersection(&white_leaf[0],&black_leaf[0],&white_leaf[1],&black_leaf[1])) {
		BlobPoint swap = black_leaf[1];
		black_leaf[1] = black_leaf[0];
		black_leaf[0] = swap;
	}
	
	// angles between black and white leaf nodes
	
	double leaf_angle_a = 0.;
	double dx_a = white_leaf[0].x - black_leaf[0].x;
	double dy_a = white_leaf[0].y - black_leaf[0].y;
	
	if( fabs(dx_a) > 0.001 ) {
		leaf_angle_a = atan(dy_a/dx_a) - M_PI * .5;
		if( dx_a < 0 ) leaf_angle_a =  leaf_angle_a - M_PI;
		if( leaf_angle_a < 0 ) leaf_angle_a += 2. * M_PI;
	} else {
		if (dy_a>0) leaf_angle_a = 0;
		else leaf_angle_a = M_PI;
	}
	
	double leaf_angle_b = 0.;
	double dx_b = white_leaf[1].x - black_leaf[1].x;
	double dy_b = white_leaf[1].y - black_leaf[1].y;
	
	if( fabs(dx_b) > 0.001 ) {
		leaf_angle_b = atan(dy_b/dx_b) - M_PI * .5;
		if( dx_b < 0 ) leaf_angle_b =  leaf_angle_b - M_PI;
		if( leaf_angle_b < 0 ) leaf_angle_b += 2. * M_PI;
	} else {
		if (dy_b>0) leaf_angle_b = 0;
		else leaf_angle_b = M_PI;
	}

	// ironing out angle jumps from 2_M_PI to 0
	double yama_angle = yama->angle;
	if (leaf_angle_a<M_PI_2 && (((yama_angle - leaf_angle_a)>M_PI) || (leaf_angle_b - leaf_angle_a)>M_PI)) leaf_angle_a+=2*M_PI;
	if (leaf_angle_b<M_PI_2 && (((yama_angle - leaf_angle_b)>M_PI) || (leaf_angle_a - leaf_angle_b)>M_PI)) leaf_angle_b+=2*M_PI;
	if (yama_angle  <M_PI_2 && (((leaf_angle_a - yama_angle)>M_PI) || (leaf_angle_b -   yama_angle)>M_PI)) yama_angle+=2*M_PI;
	
	// averaging the three angles
	double angle = (leaf_angle_a + leaf_angle_b + yama_angle)/3.0f;
	if (angle>=2*M_PI) angle-=2*M_PI;
	
	yama->angle = angle;
	yama->raw_x = (yama->raw_x + yama->root->raw_x)/2.0f;
	yama->raw_y = (yama->raw_y + yama->root->raw_y)/2.0f;

/*
#ifndef NDEBUG
	ui->setColor(255, 0, 0);
	ui->drawLine(black_leaf[0].x,black_leaf[0].y,white_leaf[0].x,white_leaf[0].y);
	ui->drawLine(black_leaf[1].x,black_leaf[1].y,white_leaf[1].x,white_leaf[1].y);
#endif
*/

	unsigned int value = 0;
	//unsigned int checksum = 0;
	bool check[] = {false, false,false,false};
	unsigned char bitpos = 0;
	double t,td,ta,tx,ty,cx,cy;
	
	double apos;
	int pixel,px,py;
	
	for (int i=0;i<6;i++) {
		
		apos = angle - M_PI_2;
		for (int p=0;p<4;p++) {
			
			t = bw/bh;
			if (t>1.1) {
				// correct elliptic blob distortion
				ta = ba-apos;
				tx = bx + cos(ta)*bw;
				ty = by + sin(ta)*bh;
				cx = tx-bx;
				cy = ty-by;
				td = sqrt(cx*cx+cy*cy)*1.5f;
			} else td = bh*1.5f; // more or less round
			
			px = (int)round(bx + cos(apos)*td);
			py = (int)round(by + sin(apos)*td);
			
			pixel = py*width+px;
			if ( (pixel<0) || (pixel>=width*height) ) {
				yama->id = FUZZY_FIDUCIAL_ID;
				return;
			}
			
#ifndef NDEBUG
			ui->setColor(255, 0, 255);
			ui->drawLine(bx,by,px,py);
#endif
			
			if (bitpos<20) {
				if (img[pixel]==0) {
					unsigned int mask = (unsigned int)pow(2,bitpos);
					value = value|mask;
					check[bitpos%4] = check[bitpos%4] ^ true;
				} else check[bitpos%4] = check[bitpos%4] ^ false;
			} else {
				bool cpix = false;
				if (img[pixel]==255) {
					//unsigned int mask = (unsigned int)pow(2,bitpos-20);
					//checksum = checksum|mask;
					cpix = true;
				}
				
				if (cpix!=check[bitpos-20]) {
					yama->id = FUZZY_FIDUCIAL_ID;
					break;
				}
			}
			
			if (invert_yamaarashi) apos-=M_PI_2;
			else apos+=M_PI_2;
			bitpos++;
		}
		
		if (invert_yamaarashi) angle -= M_PI/12.0f;
		else angle += M_PI/12.0f;
	}
	
	if (yama->id!=FUZZY_FIDUCIAL_ID) yama->id = value;
	
	/*unsigned int validation = value%13;
	if (validation==checksum) yama->id = value;
	else {
		yama->id = FUZZY_FIDUCIAL_ID;
		//std::cout << "yama cs: " << value << " " << checksum << " " << validation << std::endl;
	}*/
	
	//yama->raw_x = (yama->raw_x + bx)/2.0f;
	//yama->raw_y = (yama->raw_y + by)/2.0f;
	
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
	
	if (confidence<0.5 || confidence>1.5) {
		return 1000.0f;
	}
	
	std::vector<BlobPoint> contourList = fblob->getFullContour();
	double bx = fblob->getRawX();
	double by = fblob->getRawY();
	double bw = fblob->getRawWidth()/2.0f;
	double bh = fblob->getRawHeight()/2.0f;
	double r = 2*M_PI-fblob->getAngle();
	double Sr = sin(r);
	double Cr = cos(r);
	
	double distance = 0.0f;
	for (unsigned int i = 0; i < contourList.size(); i++) {

		double px = contourList[i].x - bx + 1;
		double py = contourList[i].y - by + 1;
		
		double pX = px*Cr - py*Sr;
		double pY = py*Cr + px*Sr;
		
		ui->setColor(0,0,255);
		ui->drawPoint(bx+pX,by+pY);
		
		double aE = atan2(pY, pX);
		
		double eX = bw * cos(aE);
		double eY = bh * sin(aE);
		
		ui->setColor(0,255,0);
		ui->drawPoint(bx+eX,by+eY);
		
		double dx = pX-eX;
		double dy = pY-eY;
		
		distance += dx*dx+dy*dy;
	}

	//std::cout << finger_sensitivity << " " << (distance/contourList.size())/bw << std::endl;
	return (distance/contourList.size())/bw;
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
	
	float min_region_size = min_object_size;
	if ((detect_fingers) && (min_region_size>min_finger_size)) min_region_size = min_finger_size;
	else if ((detect_blobs) && (min_region_size>min_blob_size)) min_region_size = min_blob_size;
	
	float max_region_size = max_blob_size;
	if (max_region_size<max_object_size) max_region_size = max_object_size;
	if (max_region_size<max_finger_size) max_region_size = max_finger_size;

	int fid_count = 0;
	int reg_count = 0;
	
	//std::cout << "fiducial size: " << min_fiducial_size << " " << max_fiducial_size << std::endl;
	//std::cout << "finger size: " << min_finger_size << " " << max_finger_size << std::endl;
	//std::cout << "region size: " << min_region_size << " " << max_region_size << std::endl;

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
		
		if( reg_count >= MAX_FIDUCIAL_COUNT*4 ) continue;
		// ignore isolated blobs without adjacent regions
		if (r->adjacent_region_count==0) continue;
		
		r->width = r->right-r->left+1;
		r->height = r->bottom-r->top+1;
		
		r->size = r->width;
		if (r->height>r->width) r->size = r->height;
		
		r->raw_x = r->left+r->width/2.0f;
		r->raw_y = r->top+r->height/2.0f;
		
		if ((r->colour==BLACK) && (!get_black_roots)) continue;
		
		if ((r->size>=min_region_size) && (r->size<=max_region_size)) {
			
			if(!empty_grid) {
				int pixel = width*(int)floor(r->y+.5f) + (int)floor(r->x+.5f);
				if ((pixel>=0) || (pixel<width*height)) {
					r->x = dmap[ pixel ].x;
					r->y = dmap[ pixel ].y;
				}
				//if ((regions[reg_count].x==0) && (regions[reg_count].y==0)) continue;
			}
			
			r->x = r->raw_x/width;
			r->y = r->raw_y/height;

			regions[reg_count] = r;
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
			
			if (fiducials[fid_count].id==YAMA_ID) {
				if (detect_yamaarashi) decodeYamaarashi(&fiducials[fid_count], dest, frameTime);
				else fiducials[fid_count].id = INVALID_FIDUCIAL_ID;
			}
			
			if ((max_fiducial_id) && (fiducials[fid_count].id>max_fiducial_id))
				fiducials[fid_count].id = FUZZY_FIDUCIAL_ID;
			
			if (fiducials[fid_count].id>=0) {
				valid_fiducial_count ++;
				total_fiducial_size += fiducials[fid_count].root->size;
				if (fiducials[fid_count].root->size < min_fiducial_size) min_fiducial_size = fiducials[fid_count].root->size;
				if (fiducials[fid_count].root->size > max_fiducial_size) max_fiducial_size = fiducials[fid_count].root->size;
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
		
		int reg_size = regions[i]->size;
		int reg_diff = abs(regions[i]->width - regions[i]->height);
		int max_diff = regions[i]->width;
		if (regions[i]->height < regions[i]->width)
			max_diff = regions[i]->height;
		
		bool add_blob = true;
		
		if ((fiducialList.size()>0) && (reg_size>=min_object_size) && (reg_size<=max_object_size) && (reg_diff < max_diff)) {
			
			if ((regions[i]->colour==WHITE) && (!get_white_roots)) continue;
			
			// ignore root blobs and adjacent regions of found fiducial roots
			if (regions[i]->flags & ROOT_REGION_FLAG) {
				add_blob = false;
			} else {
				for (int j=0;j<regions[i]->adjacent_region_count;j++) {
					if (regions[i]->adjacent_regions[j]->flags & ROOT_REGION_FLAG) {
						add_blob = false;
						break;
					}
					
				}
			}
			
			// add the root regions
			if (add_blob) {
				BlobObject *root_blob = NULL;
				try {
					root_blob = new BlobObject(frameTime,regions[i],dmap);
					rootBlobs.push_back(root_blob);
				} catch (std::exception e) { if (root_blob) delete root_blob; }
			}
			
		} else if (detect_fingers && (regions[i]->colour==WHITE) && (reg_size>=min_finger_size) && (reg_size<=max_finger_size) && reg_diff < max_diff) {
			
			//ignore noisy blobs with too many adjacencies
			//if (regions[i]->adjacent_region_count-1 >= 2*finger_sensitivity) continue;
			
			// ignore fingers that are nodes of current fiducials
			for (int j=0;j<regions[i]->adjacent_region_count;j++) {
				if (regions[i]->adjacent_regions[j]->flags & ROOT_REGION_FLAG) {
					add_blob = false;
					break;
				} else if (regions[i]->adjacent_region_count == 1) {
					for (int k=0;k<regions[i]->adjacent_regions[j]->adjacent_region_count;k++) {
						Region *test_region = regions[i]->adjacent_regions[j]->adjacent_regions[k];
						if ((test_region->flags & ROOT_REGION_FLAG) && (test_region->size>regions[i]->adjacent_regions[j]->size)) {
							add_blob = false;
							break;
						}
					}
				}
			} if (add_blob==false) continue;
			
			// ignore fingers within and near existing fiducial objects
			for (std::list<TuioObject*>::iterator tobj = objectList.begin(); tobj!=objectList.end(); tobj++) {
				
				FiducialObject *fid = (FiducialObject*)(*tobj);
				float distance = fid->getScreenDistance(regions[i]->x, regions[i]->y, width, height);
				
				if (distance < fid->getRootSize()/1.5f) {
					add_blob = false;
					break;
				}
			 }
			
			// add the finger candidates
			if (add_blob) {
				BlobObject *finger_blob = NULL;
				try {
					finger_blob = new BlobObject(frameTime,regions[i],dmap,true);
					fingerBlobs.push_back(finger_blob);
				} catch (std::exception e) { if (finger_blob) delete finger_blob; }
			}
			
		} else if (detect_blobs && (regions[i]->colour==WHITE) && (reg_size>=min_blob_size) && (reg_size>=min_blob_size)) {
			
			// add the remaining plain blob
			BlobObject *plain_blob = NULL;
			try {
				plain_blob = new BlobObject(frameTime,regions[i],dmap);
				plainBlobs.push_back(plain_blob);
			} catch (std::exception e) { if (plain_blob) delete plain_blob; }
		}
		
	}
	
	//std::cout << "roots: " << rootBlobs.size() << std::endl;
	//std::cout << "fingers: " << fingerBlobs.size() << std::endl;
	//std::cout << "blobs: " << plainBlobs.size() << std::endl;
	
	get_white_roots = false;
	get_black_roots = false;
	
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
		
		if (existing_object->getRootColour()==WHITE) get_white_roots = true;
		else get_black_roots = true;
		
		float closest = width;
		float alt_closest = width;
		FiducialX *closest_fid = NULL;
		FiducialX *alt_fid = NULL;
		
		for (std::list<FiducialX*>::iterator fid = fiducialList.begin(); fid!=fiducialList.end(); fid++) {
			FiducialX *fiducial = (*fid);
			//if (fiducial->id==INVALID_FIDUCIAL_ID) continue;
			
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
			
			existing_object->setFiducialInfo(closest_fid->root->colour,closest_fid->root->size);
			existing_object->setTrackingState(FIDUCIAL_FOUND);
			tuioManager->updateTuioObject(existing_object,closest_fid->x,closest_fid->y,closest_fid->angle);
			drawObject(existing_object->getSymbolID(),existing_object->getX(),existing_object->getY(),existing_object->getTrackingState());
			
			BlobObject *fid_blob = NULL;
			try {
				
				if ((da>M_PI/90.0f) || (dp>2)) {
					fid_blob = new BlobObject(frameTime,closest_fid->root,dmap);
					existing_object->setRootOffset(existing_object->getX()-fid_blob->getX(),existing_object->getY()-fid_blob->getY());
				}
				
				if (send_fiducial_blobs) {
					if (fid_blob==NULL) fid_blob = new BlobObject(frameTime,closest_fid->root,dmap);
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
					fid_blob = new BlobObject(frameTime,alt_fid->root,dmap);
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
					fid_blob = new BlobObject(frameTime,alt_fid->root,dmap);
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
			if (distance<fiducial->root->size/4.0f) {
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
			if (distance<fiducial->root->size/1.4f) {
				tuioManager->removeTuioCursor((*tcur));
				update_cursor_list = true;
			}
		}
		if (update_cursor_list) cursorList = tuioManager->getTuioCursors();
		
		FiducialObject *add_object = new FiducialObject(frameTime,0,fiducial->id,fiducial->x,fiducial->y,fiducial->angle);
		
		add_object->setFiducialInfo(fiducial->root->colour,fiducial->root->size);
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
			fid_blob = new BlobObject(frameTime,fiducial->root,dmap);
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

