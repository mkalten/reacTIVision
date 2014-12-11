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

#include "FingerObject.h"
#include <string>

FingerObject::FingerObject (int width, int height) {

	this->session_id    = 0;
	this->width     = (float)width;
	this->height    = (float)height;
	
	updated = false;
	alive   = false;
	state = FINGER_LOST;
	smallest_area = width*height;

	unsent = 0;
	lost_frames = 0;
	total_frames = 0;

	current.xpos = current.ypos  = -100.0f;
	current.raw_xpos = current.raw_ypos = -100.0f;
	current.motion_speed = current.motion_accel = 0.0f;
	current.motion_speed_x = current.motion_speed_y = 0.0f; 
	current.pressure = 0;
	current.time = 0;
	
	saveLastFrame();
}

FingerObject::~FingerObject() {}

void FingerObject::update (float xpos, float ypos, float area) {

	current.time = getCurrentTime();
	
	current.raw_xpos = xpos;
	current.raw_ypos = ypos;
	
	if (area < smallest_area) smallest_area = area;
	current.pressure = area/smallest_area-1;
	
	// fix possible position jitter
	positionFilter();
	// calculate speed and acceleration
	computeSpeedAccel();
	updated = true;
	lost_frames = 0;
	
	state = FINGER_FOUND;
}

void FingerObject::saveLastFrame() {

	last.time = current.time;
	last.xpos = current.xpos;
	last.ypos = current.ypos;
	last.pressure = current.pressure;
	last.motion_speed = current.motion_speed;
}

void FingerObject::positionFilter() {
	
	float threshold = 1.0f;
	
	if (fabs(current.raw_xpos-last.xpos)>threshold) current.xpos = current.raw_xpos;
	else current.xpos = last.xpos;

	if (fabs(current.raw_ypos-last.ypos)>threshold) current.ypos = current.raw_ypos;
	else current.ypos = last.ypos;
}

void FingerObject::computeSpeedAccel() {

	if (last.time==0) return;

	float dt = (current.time - last.time)/1000.0f;
	float dx = (current.xpos - last.xpos)/width;
	float dy = (current.ypos - last.ypos)/height;
	float dist = sqrt(dx*dx+dy*dy);

	current.motion_speed   = dist/dt;
	current.motion_speed_x = dx/dt;
	current.motion_speed_y = dy/dt;

	current.motion_accel   = (current.motion_speed-last.motion_speed)/dt;
}

int FingerObject::checkStatus(int s_id) {

	int message = FINGER_CANDIDATE;

	if ((!alive) && (total_frames>3)) {
		alive = true;
		session_id = s_id+1;
		return FINGER_ADDED;
	}
	
	total_frames++;
	
	if(removalFilter()) {
		if (session_id>0) message = FINGER_REMOVED;
		else  message = FINGER_EXPIRED;
	} else if (alive) message = FINGER_ALIVE;
	
	return message;
	
}

bool FingerObject::removalFilter() {
	// we remove the cursor if it hasn't appeared in the last sqrt(fps) frames

	if (!updated) {
		//int frame_threshold  = (int)floor(sqrt((float)SDLinterface::current_fps));
		int frame_threshold=2;
		if ((lost_frames>frame_threshold) || (total_frames<=3)) {
			alive = false;
			current.xpos = current.ypos  = -100.0f;
			current.raw_xpos = current.raw_ypos = -100.0f;
			current.motion_speed = current.motion_accel = 0.0f;
			current.motion_speed_x = current.motion_speed_y = 0.0f; 
			current.time = 0;
			lost_frames = 0;
			return true;
		} else {
			current.time = getCurrentTime();
			current.xpos = last.xpos;
			current.ypos = last.ypos;
			computeSpeedAccel();
			updated = true;
			lost_frames++;
			state = FINGER_LOST;
		}
	}

	return false;	
}

void FingerObject::reset() {
	lost_frames = 100;
}	

float FingerObject::distance(float x, float y) {

	// returns the distance to the current position
	float dx = x - current.xpos;
	float dy = y - current.ypos;
	return sqrt(dx*dx+dy*dy);
}

std::string FingerObject::getStatistics () {

	std::stringstream message;
	message << "cur " << session_id  <<" "<<  current.xpos <<" "<<  current.ypos <<" "<< current.pressure << " " << state;	
	return message.str();

}

std::string FingerObject::addSetMessage (TuioServer *tserver) {

	std::stringstream message;

	if(updated) {
		updated = false;
		// see if anything has changed compared to the last appearence
		if (( current.xpos!=last.xpos) || ( current.ypos!=last.ypos) ) {
			
			int s_id = session_id;
			float x = current.xpos/width;
			float y = current.ypos/height;
			float X = current.motion_speed_x;
			float Y = current.motion_speed_y;
			float m = current.motion_accel;

			unsent = 0;
			saveLastFrame();

			if(alive) {
				tserver->addCurSet(s_id,x,y,X,Y,m); 
				message << "set cur " <<" "<< s_id <<" "<< x <<" "<< y <<" "<< X <<" "<< Y <<" "<< m;
			}

			return message.str();
		} else unsent++;
	}

	saveLastFrame();
	return message.str();

}

void FingerObject::redundantSetMessage (TuioServer *server) {

	if(!alive) return;

	int s_id = session_id;
	float x = current.xpos/width;
	float y = current.ypos/height;
	float X = current.motion_speed_x;
	float Y = current.motion_speed_y;
	float m = current.motion_accel;
	
	server->addCurSet(s_id,x,y,X,Y,m);	
	unsent = 0;
}

long FingerObject::getCurrentTime() {

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
