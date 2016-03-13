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

#include "BlobMatrix.h"

const double BlobPoint::EPS = 1e-5;

BlobPoint::BlobPoint(double x, double y) {
	this->x = x;
	this->y = y;
}

BlobPoint::BlobPoint(BlobPoint *other) {
	x = other->x;
	y = other->y;
}

BlobPoint* BlobPoint::set(double x, double y) {
	this->x = x;
	this->y = y;
	return this;
}

BlobPoint* BlobPoint::set(BlobPoint *other) {
	x = other->x;
	y = other->y;
	return this;
}

double BlobPoint::get(int idx) {
	switch (idx) {
		case 0: return x;
		case 1: return y;
		default: {
			std::cout << "wrong index" << std::endl;
			return 0.0;
		}
	}
}

BlobPoint* BlobPoint::add(BlobPoint *other) {
	x += other->x;
	y += other->y;
	return this;
}

BlobPoint* BlobPoint::sub(BlobPoint *other) {
	x -= other->x;
	y -= other->y;
	return this;
}

BlobPoint* BlobPoint::scale(double s) {
	x *= s;
	y *= s;
	return  this;
}

double BlobPoint::length() {
	return sqrt(x*x + y*y);
}

double BlobPoint::lengthSqr() {
	return x*x + y*y;
}

BlobPoint* BlobPoint::normalize() {
	double len = length();
	return len >  BlobPoint::EPS ? scale(1.0/len) : this;
}

double BlobPoint::distance(BlobPoint *other) {
	double c = x-other->x;
	double d = y-other->y;
	return sqrt(c*c + d*d);
}

double BlobPoint::distanceSqr(BlobPoint *other) {
	double c = x-other->x;
	double d = y-other->y;
	return (c*c + d*d);
}

double BlobPoint::dot(BlobPoint *other) {
	return x*other->x + y*other->y;
}

bool BlobPoint::equals(BlobPoint *other) {
	
	if (fabs(x-other->x) > BlobPoint::EPS) return false;
	if (fabs(y-other->y) > BlobPoint::EPS) return false;
	return true;
}

// ---------------------------------------------------------------

BlobMatrix::BlobMatrix(BlobMatrix *other) {
	m00 = other->m00;
	m01 = other->m01;
	m10 = other->m10;
	m11 = other->m11;
}

BlobMatrix* BlobMatrix::set(BlobMatrix *other) {
	m00 = other->m00;
	m01 = other->m01;
	m10 = other->m10;
	m11 = other->m11;
	return this;
}

BlobMatrix::BlobMatrix(double m[]) {
	m00 = m[0];
	m01 = m[1];
	m10 = m[2];
	m11 = m[3];
}

BlobMatrix::BlobMatrix(double m00, double m01, double m10, double m11) {
	this->m00 = m00;
	this->m01 = m01;
	this->m10 = m10;
	this->m11 = m11;
}

BlobMatrix* BlobMatrix::setRow(int row, BlobPoint *a) {
	switch (row) {
		case 0: m00 = a->x; m01 = a->y; break;
		case 1: m10 = a->x; m11 = a->y; break;
		default: {
			std::cout << "illegal row index" << std::endl;
			return NULL;
		}
	}
	return this;
}

BlobMatrix* BlobMatrix::setColm(int colm, BlobPoint *a) {
	switch (colm) {
		case 0: m00 = a->x; m10 = a->y; break;
		case 1: m01 = a->x; m11 = a->y; break;
		default: {
			std::cout << "illegal colm index" << std::endl;
			return NULL;
		}
	}
	return this;
}

BlobMatrix* BlobMatrix::invert() {
	double d = det();
	
	if (fabs(d) < BlobMatrix::EPS) {
		std::cout << "matrix is singular" << std::endl;
		return NULL;
	}
	
	d = 1.0f/d;
	
	double p00 = d*m11;
	double p01 = -d*m01;
	double p10 = -d*m10;
	double p11 = d*m00;
	
	m00 = p00; m01 = p01;
	m10 = p10; m11 = p11;
	
	return this;
}

BlobMatrix* BlobMatrix::transpose() {
	double t = m01; m01 = m10; m10 = t;
	return this;
}

double BlobMatrix::det() {
	return m00*m11 - m01*m10;
}

BlobPoint* BlobMatrix::multiply(BlobPoint *a, BlobPoint *b) {
	b->x = m00*a->x + m01*a->y;
	b->y = m10*a->x + m11*a->y;
	return b;
}

BlobMatrix* BlobMatrix::multiply(BlobMatrix *A, BlobMatrix *B, BlobMatrix *C) {
	
	C->m00 = A->m00*B->m00 + A->m01*B->m01;
	C->m01 = A->m00*B->m01 + A->m01*B->m11;
	C->m10 = A->m10*B->m00 + A->m11*B->m01;
	C->m11 = A->m10*B->m01 + A->m11*B->m11;
	
	return C;
}

BlobMatrix* BlobMatrix::sub(BlobMatrix *other) {
	m00 -= other->m00;
	m01 -= other->m01;
	m10 -= other->m10;
	m11 -= other->m11;
	return this;
}

BlobMatrix* BlobMatrix::add(BlobMatrix *other) {
	m00 += other->m00;
	m01 += other->m01;
	m10 += other->m10;
	m11 += other->m11;
	return this;
}

const double BlobMatrix::EPS = 1e-6;

BlobPoint* BlobMatrix::solve(BlobPoint *a) {
	
	double d = m00-m10;
	
	if (fabs(d) > BlobMatrix::EPS) {
		a->y = 1.0f;
		a->x = -(m01-m11)/d;
	}
	else {
		d = m01-m11;
		if (fabs(d) > BlobMatrix::EPS) {
			a->x = 1.0f;
			a->y = -(m00-m10)/d;
		} else {
			//std::cout << "no general solution" << std::endl;
			return NULL;
		}
	}
	return a;
}

BlobMatrix* BlobMatrix::scale(double s) {
	m00 *= s;
	m01 *= s;
	m10 *= s;
	m11 *= s;
	return this;
}

double* BlobMatrix::eigenvalues() {
	return solveQuadratic(-m11-m00, det());
}

double* BlobMatrix::solveQuadratic(double p, double q) {
	double disc = p*p*0.25 - q;
	if (disc < 0.0) {
		//std::cout << "complex solution" << std::endl;
		return NULL;
	}
	disc = sqrt(disc);
	p = -p*0.5;
	//double ret[2] = { p + disc, p - disc };
	double *ret = new double[2];
	ret[0] = p + disc;
	ret[1]= p - disc;
	return ret;
}
 
 bool BlobMatrix::equals(BlobMatrix *other) {
	 if (fabs(m00-other->m00) > EPS) return false;
	 if (fabs(m01-other->m01) > EPS) return false;
	 if (fabs(m10-other->m10) > EPS) return false;
	 if (fabs(m11-other->m11) > EPS) return false;
	 return true;
 }
 
