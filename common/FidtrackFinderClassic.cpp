/*  reacTIVision tangible interaction framework
    FidtrackFinderClassic.cpp
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

#include "FidtrackFinderClassic.h"

bool FidtrackFinderClassic::init(int w, int h, int sb, int db) {
	
	FiducialFinder::init(w,h,sb,db);
	
	if (db!=1) {
		printf("target buffer must be grayscale");
		return false;
	}
	
	initialize_segmenter( &segmenter, width, height, 8 );

	return true;
}

void FidtrackFinderClassic::process(unsigned char *src, unsigned char *dest, SDL_Surface *display) {

	// segmentation
	step_segmenter( &segmenter, dest );
	// fiducial recognition
	int count = find_fiducials120( fiducials, MAX_FIDUCIAL120_COUNT,  &partialSegmentTopology, &segmenter);

	// process symbols
	for(int i=0;i< count;i++) {

		//apply distortion
		int index = (int)floor(fiducials[i].y)*width+(int)floor(fiducials[i].x);
		fiducials[i].x = dmap[index].x;
		fiducials[i].y = dmap[index].y;

		FiducialObject *existing_fiducial = NULL;

		// check if we have an ID/Position conflict
		// or correct an INVALID_FIDUCIAL120_ID if we had an ID in the last frame
		for (std::list<FiducialObject>::iterator fiducial = fiducialList.begin(); fiducial!=fiducialList.end(); fiducial++) {

			float distance = fiducial->distance(fiducials[i].x,fiducials[i].y);

			if (fiducials[i].id==fiducial->fiducial_id) {
				// find and match a fiducial we had last frame already ...
				if(existing_fiducial) {
					if (distance<existing_fiducial->distance(fiducials[i].x,fiducials[i].y))
						existing_fiducial = &(*fiducial);
				} else {
					existing_fiducial = &(*fiducial);
					for (int j=0;j<count;j++) {
						if ((i!=j) && (fiducial->distance(fiducials[j].x,fiducials[j].y)<distance)) {
							existing_fiducial = NULL;
							break;
						}
					}	
				}
			} else  if (distance<5.0f) {
				// do we have a different ID at the same place?
	
				// this should correct wrong or invalid fiducial IDs
				// assuming that between two frames
				// there can't be a rapid exchange of tw symbols
				// at the same place	

				/*if (fiducials[i].id==INVALID_FIDUCIAL120_ID) printf("corrected invalid ID to %d at %f %f\n",fiducials[i].id,fiducials[i].x/width,fiducials[i].y/height);
				else if (fiducials[i].id!=pos->classId) printf("corrected wrong ID from %d to %d at %f %f\n",fiducials[i].id,pos->classId,fiducials[i].x/width,fiducials[i].y/height);*/

				fiducials[i].id=fiducial->fiducial_id;
				existing_fiducial = &(*fiducial);
				break;
			}
		}

		if  (existing_fiducial!=NULL) {
			// just update the fiducial from last frame ...
			existing_fiducial->update(fiducials[i].x,fiducials[i].y,fiducials[i].angle,0,0);
		} else if  (fiducials[i].id!=INVALID_FIDUCIAL120_ID) {
			// add the newly found object
			FiducialObject addFiducial(session_id, fiducials[i].id, width, height);
			addFiducial.update(fiducials[i].x,fiducials[i].y,fiducials[i].angle,0,0);
			fiducialList.push_back(addFiducial);
			if (midi_server!=NULL) midi_server->sendAddMessage(fiducials[i].id);
			if (msg_listener) {
				//char add_message[16];
				//sprintf(add_message,"add obj %d %ld",fiducials[i].id,session_id);
				//msg_listener->setMessage(std::string(add_message));
				std::stringstream add_message;
				add_message << "add obj " << fiducials[i].id << " " << session_id;
				msg_listener->setMessage(add_message.str());
			}
			session_id++;
		}

		if (fiducials[i].id!=INVALID_FIDUCIAL120_ID) drawObject(fiducials[i].id,(int)(fiducials[i].x),(int)(fiducials[i].y),display,1);

	} 

	if (tuio_server) sendTuioMessages();
	if (midi_server) sendMidiMessages();
	if (show_grid) drawGrid(src,dest,display);
	if (show_settings) drawGUI(display);
}


