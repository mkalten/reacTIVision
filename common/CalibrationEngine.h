/*  reacTIVision tangible interaction framework
    CalibrationEngine.h
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

#ifndef CALIBRATIONENGINE_H
#define CALIBRATIONENGINE_H

#include "FrameProcessor.h"
#include "FidtrackFinder.h"
#include "CalibrationGrid.h"
#include <vector>

struct PointPair {

	FloatPoint original;
	FloatPoint rotation;
	float angle;
};

class CalibrationEngine: public FrameProcessor
{
public:
	CalibrationEngine(const char* out);
	~CalibrationEngine();

	void process(unsigned char *src, unsigned char *dest, SDL_Surface *display);
	void drawDisplay(unsigned char* display);
	bool init(int w ,int h, int sb, int db);

	void setFlag(int flag, bool value);
	void toggleFlag(int flag);

private:
	CalibrationGrid *grid;
	const char* calib_out;
	char calib_bak[1024];
	#ifdef __APPLE__
	char full_path[1024];
	#endif
	bool file_exists;

	bool calibration;
    bool quick_mode;

	int grid_xpos, grid_ypos;
	int grid_size_x, grid_size_y;
	int field_count_x, field_count_y;
    int center_x, center_y;
	int cell_size_x, cell_size_y;

	MessageListener::DisplayMode prevMode;
};

#endif
