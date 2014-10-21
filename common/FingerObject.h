/*  reacTIVision tangible interaction framework
    FingerObject.h
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

#ifndef FINGEROBJECT_H
#define FINGEROBJECT_H

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <sstream>
#include <string>

#include "TuioServer.h"
#include "MidiServer.h"
#include "SDLinterface.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

#define DOUBLEPI 6.283185307179586

#define FINGER_CANDIDATE 0
#define FINGER_ALIVE 1
#define FINGER_ADDED 2
#define FINGER_REMOVED 3
#define FINGER_EXPIRED 4

#define FINGER_LOST 0
#define FINGER_FOUND 1

struct cframe {
	float xpos,ypos;
	float raw_xpos, raw_ypos;
	float motion_speed, motion_accel;
	float motion_speed_x, motion_speed_y;
	float pressure;
	long time;
};

class FingerObject {
  public:
	bool alive;
	int unsent;
	int session_id;
	int state;
	float smallest_area;

  private:
	bool updated;
	int lost_frames;
	
	float width;
	float height;
	
	cframe current, last;
	long total_frames;

	void positionFilter();
	void computeSpeedAccel();
	bool removalFilter();
	void saveLastFrame();
	long getCurrentTime();

  public:
	FingerObject(int width, int height);
	~FingerObject();
	void update(float xpos, float ypos, float area);
	std::string addSetMessage(TuioServer *tserver);
	void redundantSetMessage(TuioServer *server);
	std::string getStatistics ();
	
	int checkStatus(int s_id);
	float distance(float x, float y);
	void reset();
	float getX() { return current.xpos; };
	float getY() { return current.ypos; };
};


#endif
