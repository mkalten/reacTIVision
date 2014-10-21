/*
	CalibrationGrid.h - part of the reacTIVision project
	Copyright (c) 2006 Marcos Alonso <malonso@upf.edu>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

struct GridPoint
{
	double x,y;
};

class CalibrationGrid
{
public:
	CalibrationGrid(int width,int height)
		: points_((width+1)*(height+1))
	{
		this->width = width;
		this->height = height;
	};

	int GetWidth() { return width; }
	int GetHeight() { return height; }

	GridPoint Get(int x, int y);
	void Set(int x, int y, double vx, double vy);

	bool IsEmpty();
	void Reset();
	void Load(const char* filename);
	void Store(const char* filename);

	GridPoint GetInterpolated( float x, float y );

private:
	int width,height;
	std::vector<GridPoint> points_;

	GridPoint GetInterpolatedX( float x, int y );
	GridPoint GetInterpolatedY( int x, float y );
	inline double catmullRomSpline(double x,double v1,double v2,double v3,double v4);
};
