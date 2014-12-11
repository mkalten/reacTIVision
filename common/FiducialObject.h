/*  reacTIVision tangible interaction framework
    FiducialObject.h
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

#ifndef FIDOBJECT_H
#define FIDOBJECT_H

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "TuioServer.h"
#include "MidiServer.h"
#include "SDLinterface.h"
#include "floatpoint.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

#define DOUBLEPI 6.283185307179586

#define FIDUCIAL_LOST 0
#define FIDUCIAL_FOUND 1
#define FIDUCIAL_INVALID 2
#define FIDUCIAL_WRONG 3
#define FIDUCIAL_REGION 4

struct fframe {
	float xpos,ypos,angle;
	float raw_xpos, raw_ypos, raw_angle;
	float rotation_speed, rotation_accel;
	float motion_speed, motion_accel;
	float motion_speed_x, motion_speed_y;
	long time;
};

class FiducialObject {
  public:
	bool alive;
	int unsent;
	int session_id;
	int fiducial_id;
	float getAngle() { return current.angle; }
	float getX() { return current.xpos; }
	float getY() { return current.ypos; }
	bool isUpdated() { return updated; }
	bool checkIdConflict(int s_id, int f_id);
	int state;
	float root_size, leaf_size;
	int root_colour;
	int node_count;
	
  private:
	bool updated;
	int lost_frames;
	
	float width;
	float height;
	
	fframe current, last;
	
	void positionFilter();
	void computeSpeedAccel();
	bool removalFilter();
	void saveLastFrame();
	long getCurrentTime();
	
	FloatPoint blob_offset;
	
  public:
	FiducialObject(int s_id, int f_id, int width, int height);
	FiducialObject(int s_id, int f_id, int width, int height, int colour, int node_count);
	~FiducialObject();
	void update(float x, float y, float a, float root, float leaf);
	std::string getStatistics();
	std::string addSetMessage(TuioServer *tserver);
	std::string addSetMessage(MidiServer *mserver);
	void redundantSetMessage(TuioServer *server);

	void setBlobOffset(float x, float y);
	FloatPoint getBlobOffset();
	
	std::string checkRemoved();
	float distance(float x, float y);
	int id_buffer[6];
	short id_buffer_index;
};


#endif
