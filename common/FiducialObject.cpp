/*  reacTIVision tangible interaction framework
    FiducialObject.cpp
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

#include "FiducialObject.h"
#include <string>
#include <sstream>

FiducialObject::FiducialObject (int s_id, int f_id, int width, int height, int colour, int nodes) {
	this->session_id    = s_id,
	this->fiducial_id   = f_id;
	this->width     = (float)width;
	this->height    = (float)height;
	this->root_colour = colour;
	this->node_count = nodes;
	
	updated = false;
	alive   = false;
	state = FIDUCIAL_LOST;
	
	unsent = 0;
	lost_frames = 0;
	
	current.xpos = current.ypos = current.raw_xpos = current.raw_ypos = -100.0f;
	current.angle = current.raw_angle = 0.0f;
	current.rotation_speed = current.rotation_accel = 0.0f;
	current.motion_speed = current.motion_accel = 0.0f;
	current.motion_speed_x = current.motion_speed_y = 0.0f; 
	current.time = 0;
	
	blob_offset.x = 0.0f;
	blob_offset.y = 0.0f;
	
	id_buffer_index=0;
	
	saveLastFrame();
}

FiducialObject::FiducialObject (int s_id, int f_id, int width, int height) {

	this->session_id    = s_id,
	this->fiducial_id   = f_id;
	this->width     = (float)width;
	this->height    = (float)height;
	this->root_colour = 255;
	this->node_count = 0;

	updated = false;
	alive   = false;
	state = FIDUCIAL_LOST;

	unsent = 0;
	lost_frames = 0;
	
	current.xpos = current.ypos = current.raw_xpos = current.raw_ypos = -100.0f;
	current.angle = current.raw_angle = 0.0f;
	current.rotation_speed = current.rotation_accel = 0.0f;
	current.motion_speed = current.motion_accel = 0.0f;
	current.motion_speed_x = current.motion_speed_y = 0.0f; 
	current.time = 0;

	id_buffer_index=0;

	saveLastFrame();
}

FiducialObject::~FiducialObject() {}

void FiducialObject::update (float x, float y, float a, float root, float leaf) {

	current.time = getCurrentTime();
	
	current.raw_xpos = x;
	current.raw_ypos = y;
	current.raw_angle = a;
	
	root_size = root;
	leaf_size = leaf;
	
	// fix possible position and angle jitter
	positionFilter();
	// calculate movement and rotation speed and acceleration
	computeSpeedAccel();
	
	state = FIDUCIAL_FOUND;
	updated = true;
	alive = true;
	lost_frames = 0;

}

void FiducialObject::setBlobOffset (float x, float y) {
	blob_offset.x = x;
	blob_offset.y = y;
}

FloatPoint FiducialObject::getBlobOffset () {
	return blob_offset;
}

void FiducialObject::saveLastFrame() {

	
	last.time = current.time;
	last.xpos = current.xpos;
	last.ypos = current.ypos;
	last.angle = current.angle;
	last.motion_speed = current.motion_speed;
	last.rotation_speed = current.rotation_speed;
}

void FiducialObject::positionFilter() {
	
	// one pixel threshold
	if (fabs(current.raw_xpos-last.xpos)>1.0f) current.xpos = current.raw_xpos;
	else current.xpos = last.xpos;

	// one pixel threshold
	if (fabs(current.raw_ypos-last.ypos)>1.0f) current.ypos = current.raw_ypos;
	else current.ypos = last.ypos;

	// three degree threshold
	if (fabs(current.raw_angle-last.angle)>DOUBLEPI/120.0f) current.angle = current.raw_angle;
	else current.angle = last.angle;
}

void FiducialObject::computeSpeedAccel() {

	if (last.time==0) return;

	float dt = (current.time - last.time)/1000.0f;
	float dx = (current.xpos - last.xpos)/width;
	float dy = (current.ypos - last.ypos)/height;
	float dist = sqrt(dx*dx+dy*dy);
	
	float da = current.angle-last.angle;
	if (da>(M_PI*1.5f)) da-=DOUBLEPI;
	else if (da<(M_PI*-1.5f)) da+=DOUBLEPI;
	da = da/DOUBLEPI;
	
	current.motion_speed   = dist/dt;
	current.motion_speed_x = dx/dt;
	current.motion_speed_y = dy/dt;
	current.rotation_speed = da/dt;

	current.motion_accel   = (current.motion_speed   - last.motion_speed)/dt;
	current.rotation_accel = (current.rotation_speed - last.rotation_speed)/dt;
}

std::string FiducialObject::checkRemoved() {

	std::stringstream message;
	if(removalFilter()) message << "del obj " << session_id << " " << fiducial_id;
	return message.str();
}

bool FiducialObject::removalFilter() {
	// we remove an object if it hasn't appeared in the last n frames
	if (!updated) {
		int frame_threshold = 2;
		if (lost_frames>frame_threshold) {
			alive = false;
			current.xpos = current.ypos = current.raw_xpos = current.raw_ypos = -100.0f;
			current.angle = current.raw_angle = 0.0f;
			current.rotation_speed = current.rotation_accel = 0.0f;
			current.motion_speed = current.motion_accel = 0.0f;
			current.motion_speed_x = current.motion_speed_y = 0.0f; 
			current.time = 0;
			lost_frames = 0;
			return true;
		} else {
			current.time = getCurrentTime();
			current.xpos = last.xpos;
			current.ypos = last.ypos;
			current.angle = last.angle;
			computeSpeedAccel();
			updated = true;
			lost_frames++;
			state = FIDUCIAL_LOST;
		}
	}

	return false;	
}

float FiducialObject::distance(float x, float y) {

	// returns the distance to the current position
	float dx = x - current.xpos;
	float dy = y - current.ypos;
	return sqrt(dx*dx+dy*dy);
}

std::string FiducialObject::addSetMessage (TuioServer *tserver) {

	std::stringstream message;

	if(updated) {
		updated = false;
		// see if anything has changed compared to the last appearence
		if (( current.xpos!=last.xpos) || ( current.ypos!=last.ypos) || ( current.angle!=last.angle) ) {
			
			int s_id = session_id;
			int f_id = fiducial_id;
			float x = current.xpos/width;
			float y = current.ypos/height;
			float a = current.angle;
			float X = current.motion_speed_x;
			float Y = current.motion_speed_y;
			float A = current.rotation_speed;
			float m = current.motion_accel;
			float r = current.rotation_accel;
	
			tserver->addObjSet(s_id,f_id,x,y,a,X,Y,A,m,r);
			message << "set obj " <<" "<< s_id <<" "<< f_id <<" "<< x <<" "<< y <<" "<< a <<" "<< X <<" "<< Y <<" "<< A <<" "<< m <<" "<< r;	

			unsent = 0;
			saveLastFrame();
			return message.str();
		} else unsent++;
	}

	saveLastFrame();
	return message.str();

}


std::string FiducialObject::getStatistics () {

	std::stringstream message;
	message << "obj " << session_id <<" "<< fiducial_id <<" "<<  current.xpos <<" "<<  current.ypos <<" "<<  current.angle <<" "<< state;	
	return message.str();

}

bool FiducialObject::checkIdConflict (int s_id, int f_id) {

	id_buffer[id_buffer_index] = f_id;

	int error=0;
	for (int i=0;i<6;i++) {
		int index = i+id_buffer_index;
		if(index==6) index-=6;
		if (id_buffer[index]==f_id) error++;
	}

	id_buffer_index++;
	if (id_buffer_index==6) id_buffer_index=0;

	if (error>4) {
		//printf("--> corrected wrong object ID from %d to %d\n",fiducial_id,f_id);
		fiducial_id=f_id;
		session_id=s_id+1;
		state = FIDUCIAL_WRONG;
		return true;
	}
	return false;
}


std::string FiducialObject::addSetMessage (MidiServer *mserver) {

	std::stringstream message;

	if(updated) {
		updated = false;
		// see if anything has changed compared to the last appearance
		if (( current.xpos!=last.xpos) || ( current.ypos!=last.ypos) || ( current.angle!=last.angle) || (lost_frames>0) ) {
			
			int s_id = session_id;
			int f_id = fiducial_id;
			float x = current.xpos/width;
			float y = current.ypos/height;
			float a = current.angle;
			float X = current.motion_speed_x;
			float Y = current.motion_speed_y;
			float A = current.rotation_speed;
			float m = current.motion_accel;
			float r = current.rotation_accel;
	
			mserver->sendControlMessage(f_id, x, y, a);
			message << "set obj " <<" "<< s_id <<" "<< f_id <<" "<< x <<" "<< y <<" "<< a <<" "<< X <<" "<< Y <<" "<< A <<" "<< m <<" "<< r;	

			unsent = 0;
			saveLastFrame();
			return message.str();
		} else unsent++;
	}

	saveLastFrame();
	return message.str();
}



void FiducialObject::redundantSetMessage (TuioServer *server) {

	int s_id = session_id;
	int f_id = fiducial_id;
	float x = current.xpos/width;
	float y = current.ypos/height;
	float a = current.angle;
	float X = current.motion_speed_x;
	float Y = current.motion_speed_y;
	float A = current.rotation_speed;
	float m = current.motion_accel;
	float r = current.rotation_accel;
	
	server->addObjSet(s_id,f_id,x,y,a,X,Y,A,m,r);	
	unsent = 0;
}

long FiducialObject::getCurrentTime() {

	#ifdef WIN32
		long timestamp = GetTickCount();
	#else
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv,&tz);
		long timestamp = (tv.tv_sec*1000)+(tv.tv_usec/1000);
	#endif
	
	return timestamp;
}
