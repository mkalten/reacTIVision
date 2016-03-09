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

#ifndef BLOBMATRIX_H
#define BLOBMATRIX_H

#include "math.h"
#include <iostream>

typedef struct BlobSpan {
	int start, end;
	BlobSpan *next;
} BlobSpan;

// ---------------------------------------------------------------

class BlobPoint
{
public:
	double x;
	double y;
	static const double EPS;
	
	BlobPoint(): x(0.0),y(0.0) {};
	~BlobPoint() {}
	
	BlobPoint(double x, double y);
	BlobPoint(BlobPoint *other);
	BlobPoint* set(double x, double y);
	BlobPoint* set(BlobPoint *other);
	double get(int idx);
	BlobPoint* add(BlobPoint *other);
	BlobPoint* sub(BlobPoint *other);
	BlobPoint* scale(double s);
	double length();
	double lengthSqr();
	BlobPoint* normalize();
	double distance(BlobPoint *other);
	double distanceSqr(BlobPoint *other);
	double dot(BlobPoint *other);
	bool equals(BlobPoint *other);
};

// ---------------------------------------------------------------

class BlobMatrix {
public:
	static const double EPS;// = 1e-6;
	double m00, m01, m10, m11;
	//static const BlobMatrix identity = (1.0f, 0.0f, 0.0f, 1.0f);
	
	BlobMatrix(): m00(0.0f), m01(0.0f), m10(0.0f), m11(0.0f) {}
	~BlobMatrix() {}
	
	BlobMatrix(BlobMatrix *other);
	BlobMatrix(double m[]);
	BlobMatrix(double m00, double m01, double m10, double m11);
	BlobMatrix* set(BlobMatrix *other);
	BlobMatrix* setRow(int row, BlobPoint *a);
	BlobMatrix* setColm(int colm, BlobPoint *a);
	BlobMatrix* invert();
	BlobMatrix* transpose();
	double det();
	BlobPoint* multiply(BlobPoint *a, BlobPoint *b);
	BlobMatrix* multiply(BlobMatrix *A, BlobMatrix *B, BlobMatrix *C);
	BlobMatrix* sub(BlobMatrix *other);
	BlobMatrix* add(BlobMatrix *other);
	BlobPoint* solve(BlobPoint *a);
	BlobMatrix* scale(double s);
	double* eigenvalues();
	double* solveQuadratic(double p, double q);
	bool equals(BlobMatrix *other);
};
#endif
