/*
	CalibrationGrid.cpp - part of the reacTIVision project
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

#include "CalibrationGrid.h"

GridPoint CalibrationGrid::Get(int x, int y) 
{ 
	if( x>=0 && x<width && y>=0 && y<height ) return points_[y*width+x];

	GridPoint npoint;
	npoint.x = 0;
	npoint.y = 0;
	return npoint;
}

void CalibrationGrid::Set(int x, int y, double vx, double vy)
{
	if( x>=0 && x<width && y>=0 && y<height )
	{
		points_[y*width+x].x = vx;
		points_[y*width+x].y = vy;
	}
}


bool CalibrationGrid::IsEmpty()
{
	for( unsigned int i=0; i<points_.size(); i++ )
	{
		if (points_[i].x != 0) return false;
		if (points_[i].y != 0) return false;
	}
	
	return true;
}

void CalibrationGrid::Reset()
{
	for( unsigned int i=0; i<points_.size(); i++ )
	{
		points_[i].x = 0;
		points_[i].y = 0;
	}
}

void CalibrationGrid::Load(const char* filename)
{
	std::fstream file(filename,std::ios::in);
	for( unsigned int i=0; i<points_.size(); i++ )
	{
		file >> points_[i].x;
		file >> points_[i].y;
	}
	file.close();
}

void CalibrationGrid::Store(const char* filename)
{
	std::fstream file(filename,std::ios::out);
	for( unsigned int i=0; i<points_.size(); i++ )
	{
		file << points_[i].x << " " << points_[i].y << "\n";
	}
	file.close();
}

GridPoint CalibrationGrid::GetInterpolated( float x, float y )
{
	GridPoint point;

	if( x>=width ) return GetInterpolatedY(width,y);
	if( y>=height ) return GetInterpolatedX(x,height);

	// x
	int x_floor = (int)floor(x);
	float x_offset = x - x_floor;

	GridPoint x1 = GetInterpolatedY(x_floor,y);
	GridPoint x2 = GetInterpolatedY(x_floor+1,y);
	point.x = (x1.x * (1-x_offset)) + (x2.x * x_offset);

	// y
	int y_floor = (int)floor(y);
	float y_offset = y - y_floor;

	GridPoint y1 = GetInterpolatedX(x,y_floor);
	GridPoint y2 = GetInterpolatedX(x,y_floor+1);
	point.y = (y1.y * (1-y_offset)) + (y2.y * y_offset);

	return point;
}

GridPoint CalibrationGrid::GetInterpolatedX( float x, int y )
{
	int x_floor = (int)floor(x);
	float x_offset = x - x_floor;

	GridPoint v1;
	if( x_floor<=0 ) v1 = points_[ (y*width) + x_floor ];
	else v1 = points_[ (y*width) + x_floor-1 ];

	GridPoint v2 = points_[ (y*width) + x_floor ];

	GridPoint v3 = points_[ (y*width) + x_floor+1 ];

	GridPoint v4;
	if( x_floor>=(width-2) ) v4 = points_[ (y*width) + x_floor+1 ];
	else v4 = points_[ (y*width) + x_floor+2 ];

	GridPoint point;
	point.x = catmullRomSpline( x_offset, v1.x, v2.x, v3.x, v4.x );
	point.y = catmullRomSpline( x_offset, v1.y, v2.y, v3.y, v4.y );

	return point;
}

GridPoint CalibrationGrid::GetInterpolatedY( int x, float y )
{
	int y_floor = (int)floor(y);
	float y_offset = y - y_floor;

	GridPoint v1;
	if( y_floor<=0 ) v1 = points_[ (y_floor*width) + x ];
	else v1 = points_[ ((y_floor-1) * width) + x ];

	GridPoint v2 = points_[ (y_floor * width) + x ];

	GridPoint v3 = points_[ ((y_floor+1) * width) + x ];

	GridPoint v4;
	if( y_floor>=(height-2) ) v4 = points_[ ((y_floor+1) * width) + x ];
	else v4 = points_[ ((y_floor+2) * width) + x ];


	GridPoint point;
	point.x = catmullRomSpline( y_offset, v1.x, v2.x, v3.x, v4.x );
	point.y = catmullRomSpline( y_offset, v1.y, v2.y, v3.y, v4.y );

	return point;
}

inline double CalibrationGrid::catmullRomSpline(double x,double v1,double v2,double v3,double v4)
{
	double c1,c2,c3,c4;

	c1 =  1.0*v2;
	c2 = -0.5*v1 +  0.5*v3;
	c3 =  1.0*v1 + -2.5*v2 +  2.0*v3 + -0.5*v4;
	c4 = -0.5*v1 +  1.5*v2 + -1.5*v3 +  0.5*v4;

	return (((c4*x + c3)*x +c2)*x + c1);
}
