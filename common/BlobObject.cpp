/*	reacTIVision tangible interaction framework
	Copyright (C) 2005-2017 Martin Kaltenbrunner <martin@tuio.org>
 
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

#include "BlobObject.h"
using namespace TUIO;

int BlobObject::screenWidth = WIDTH;
int BlobObject::screenHeight = HEIGHT;
UserInterface* BlobObject::ui = NULL;

BlobObject::BlobObject(TuioTime ttime, Region *region, ShortPoint *dmap, bool do_full_analyis):TuioBlob(ttime, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f) {
	
	if (region==NULL) throw std::exception();
	blobRegion = region;
	
	/*
	#ifndef NDEBUG
	//fill the region candidate with yellow
	Span *span = blobRegion->first_span;
	ui->setColor(255, 255, 0);
	while (span) {
		ui->drawLine(span->start%screenWidth,span->start/screenWidth,span->end%screenWidth,span->end/screenWidth);
		span = span->next;
	}
	 
	//draw a blue rectangle around the region candidate
	ui->setColor(0,0,255);
	ui->drawRect(region->left,region->top,region->right-region->left,region->bottom-region->top);
	#endif
	//*/
	
	computeInnerSpanList();
	computeOuterContourList();
	if (outerContour.size()==0) throw std::exception();
	
	//std::vector<BlobPoint> innerContourList = getInnerContourList(region);
	//if (outerContourList.size()==0) return NULL;
	
	computeConvexHull();
	if (convexHull.size()==0) throw std::exception();
	
	if (do_full_analyis) {
		computeSpanList();
		computeFullContourList();
		if (fullContour.size()==0) throw std::exception();
		
#ifndef NDEBUG
		ui->setColor(255,0,255);
		for (unsigned int i=0; i<fullContour.size(); i++)
			ui->drawPoint(fullContour[i].x, fullContour[i].y);
#endif

	} else {
		
#ifndef NDEBUG
		ui->setColor(255,0,0);
		BlobPoint ch_pt = convexHull[0];
		for (unsigned int i = 1; i < convexHull.size(); i++) {
		 BlobPoint pt1 = convexHull[i];
		 ui->drawLine(ch_pt.x,ch_pt.y,pt1.x,pt1.y);
		 ch_pt.x = pt1.x;
		 ch_pt.y = pt1.y;
		}
		BlobPoint pt1 = convexHull[0];
		ui->drawLine(ch_pt.x,ch_pt.y,pt1.x,pt1.y);
#endif
		
	}
	
	computeOrientedBoundingBox();
	if (obBox.size()==0) throw std::exception();
	
/*
	 #ifndef NDEBUG
	 ui->setColor(0,255,0);
	 for (int i=0;i<4;i++) {
		int j = (i<3) ? i+1:0;
		ui->drawLine(obBox[i].x,obBox[i].y,obBox[j].x, obBox[j].y);
	 }
	 #endif
*/
	
	rawXpos = obBox[4].x;
	rawYpos = obBox[4].y;
	
	if(dmap) {
		int pixel = screenWidth*(int)floor(rawYpos+.5f) + (int)floor(rawXpos+.5f);
		if ((pixel>=0) || (pixel<screenWidth*screenHeight)) {
			xpos = dmap[ pixel ].x/(float)screenWidth;
			ypos = dmap[ pixel ].y/(float)screenHeight;
		}
	} else {
		xpos = rawXpos/screenWidth;
		ypos = rawYpos/screenHeight;
	}
	
	rawWidth  = obBox[5].x;
	rawHeight = obBox[5].y;
	
	BlobPoint *bottom_left = &obBox[0];
	BlobPoint *bottom_right = &obBox[0];
	
	for (int i=1;i<4;i++) {
		if ((obBox[i].x<bottom_left->x) && (obBox[i].y<bottom_left->y)) bottom_left = &obBox[i];
		if ((obBox[i].x>bottom_right->x) && (obBox[i].y<bottom_right->y)) bottom_right = &obBox[i];
	}

	double ak = bottom_right->x - bottom_left->x;
	double hp  = bottom_left->distance(bottom_right);
	angle = 2*M_PI-acos(ak/hp);

	if (hp<rawHeight) {
		rawWidth  = obBox[5].y;
		rawHeight = obBox[5].x;
		angle-=M_PI_2;
		if (angle<0) angle += 2*M_PI;
	}
	
	width = rawWidth/screenWidth;
	height = rawHeight/screenHeight;
	area = (float)region->area/(screenWidth*screenHeight);

/*
#ifndef NDEBUG
	ui->setColor(0,255,0);
	int px = (int)roundf(rawXpos + cos(angle)*rawWidth/2.0f);
	int py = (int)roundf(rawYpos + sin(angle)*rawHeight/2.0f);
	ui->drawLine(rawXpos,rawYpos,px,py);
#endif
*/
	
}

void BlobObject::computeInnerSpanList() {
	for (int s=0; s<blobRegion->adjacent_region_count;s++) {
		if ((blobRegion->adjacent_regions[s]->right-blobRegion->adjacent_regions[s]->left)<blobRegion->width) {
			innerSpanList.push_back(blobRegion->adjacent_regions[s]->first_span);
		}
	}
	
	//std::cout << "inner spans: " << innerSpanList.size() << std::endl;
}

void BlobObject::computeOuterContourList() {
	
	Span *span = blobRegion->first_span;
	while (span) {
		
		bool add_start = true;
		bool add_end = true;
		
		for (int i=0;i<innerSpanList.size();i++) {
			Span *inner = innerSpanList[i];
			while (inner) {
				if (span->end+1==inner->start) {
					add_end=false;
					if(!add_start) i=innerSpanList.size();
					break;
				} else if (span->start-1==inner->end) {
					add_start=false;
					if(!add_end) i=innerSpanList.size();
					break;
				}
				inner = inner->next;
			}
		}
		
		if (add_start) outerContour.push_back(BlobPoint(span->start%screenWidth, span->start/screenWidth));
		if (add_end)   outerContour.push_back(BlobPoint(span->end%screenWidth, span->end/screenWidth));

		span = span->next;
	}
	
	std::sort(outerContour.begin(), outerContour.end());
	//std::cout << "outer contour: " << outerContour.size() << std::endl;
}

void BlobObject::computeFullContourList() {
	
	for (unsigned int row=0;row<spanList.size();row++) {
		
		BlobSpan *row_span = &spanList[row];
		
		
		if ((row==0) || (row==spanList.size()-1)) {
			
			while (row_span) {
				
				for (int i=row_span->start;i<=row_span->end;i++) {
					//BlobPoint spanPoint(i%screenWidth, i/screenWidth);
					fullContour.push_back(BlobPoint(i%screenWidth, i/screenWidth));
					
					//ui->setColor(255,0,0);
					//ui->drawPoint(spanPoint.x,spanPoint.y);
				}
				
				row_span = row_span->next;
			}
			
		} else {
			
			while (row_span) {
				
				BlobPoint startPoint(row_span->start%screenWidth, row_span->start/screenWidth);
				BlobPoint endPoint(row_span->end%screenWidth,row_span->end/screenWidth);
				fullContour.push_back(startPoint);
				
				//ui->setColor(0,0,255);
				//ui->drawPoint(startPoint.x,startPoint.y);
				
				BlobSpan *upper_span = &spanList[row-1];
				int start = row_span->start+1;
				
				while (upper_span) {
					
					int end = upper_span->start+screenWidth;
					
					for (int pos=start;pos<end;pos++) {
						BlobPoint fillPoint;
						fillPoint.x = pos%screenWidth;
						if ((fillPoint.x>startPoint.x) && (fillPoint.x<endPoint.x)) {
							fillPoint.y = pos/screenWidth;
							fullContour.push_back(fillPoint);
							
							//ui->setColor(255,0,0);
							//ui->drawPoint(fillPoint.x,fillPoint.y);
						}
					}
					
					start = upper_span->end+screenWidth+1;
					upper_span = upper_span->next;
				}
				
				int end = row_span->end;
				for (int pos=start;pos<end;pos++) {
					BlobPoint fillPoint;
					fillPoint.x = pos%screenWidth;
					if ((fillPoint.x>startPoint.x) && (fillPoint.x<endPoint.x)) {
						fillPoint.y = pos/screenWidth;
						fullContour.push_back(fillPoint);
						
						//ui->setColor(255,0,0);
						//ui->drawPoint(fillPoint.x,fillPoint.y);
					}
				}
				
				
				
				BlobSpan *lower_span = &spanList[row+1];
				
				
				start = row_span->start+1;
				
				while (lower_span) {
					
					int end = lower_span->start-screenWidth;
					
					for (int pos=start;pos<end;pos++) {
						BlobPoint fillPoint;
						fillPoint.x = pos%screenWidth;
						if ((fillPoint.x>startPoint.x) && (fillPoint.x<endPoint.x)) {
							fillPoint.y = pos/screenWidth;
							fullContour.push_back(fillPoint);
							
							//ui->setColor(255,0,0);
							//ui->drawPoint(fillPoint.x,fillPoint.y);
						}
					}
					
					start = lower_span->end+-screenWidth+1;
					lower_span = lower_span->next;
				}
				
				end = row_span->end;
				for (int pos=start;pos<end;pos++) {
					BlobPoint fillPoint;
					fillPoint.x = pos%screenWidth;
					if ((fillPoint.x>startPoint.x) && (fillPoint.x<endPoint.x)) {
						fillPoint.y = pos/screenWidth;
						fullContour.push_back(fillPoint);
						
						//ui->setColor(255,0,0);
						//ui->drawPoint(fillPoint.x,fillPoint.y);
					}
				}
				
				
				fullContour.push_back(endPoint);
				
				//ui->setColor(0,0,255);
				//ui->drawPoint(endPoint.x,endPoint.y);
				
				row_span = row_span->next;
			}
			
		}
		
	}
	
}

void BlobObject::computeSpanList() {
	
	//spanList.clear();
	//fullSpanList.clear();
	//sortedSpanList.clear();
	
	Span *span = blobRegion->first_span;
	sortedSpanList.push_back(span);
	span = span->next;
	while (span) {
		
		if (span->start>sortedSpanList.back()->start)
			sortedSpanList.push_back(span);
		else if (span->start<sortedSpanList.front()->start)
			sortedSpanList.push_front(span);
		else {
			for (std::list<Span*>::iterator iter = sortedSpanList.begin(); iter != sortedSpanList.end(); iter++) {
				if (span->start<(*iter)->start) {
					sortedSpanList.insert(iter, span);
					break;
				}
			}
		}
		span = span->next;
	}
	
	int i=-1;
	int last_y = -1;
	for (std::list<Span*>::iterator span = sortedSpanList.begin(); span != sortedSpanList.end(); span++) {
		
		int row_y = (*span)->start/screenWidth;
		BlobSpan nspan = { (*span)->start, (*span)->end };
		nspan.next = NULL;
		fullSpanList.push_back(nspan);
		
		if (row_y>last_y) {
			spanList.push_back(nspan);
			last_y = row_y;
			i++;
		} else {
			BlobSpan *pspan = &spanList[i];
			while (pspan->next!=NULL) pspan=pspan->next;
			pspan->next = &fullSpanList.back();
		}
	}
}

std::vector<BlobPoint> BlobObject::getInnerContourList() {
	std::vector<BlobPoint> pointList;
	
	for (int i=0;i<innerSpanList.size();i++) {
		Span *span = innerSpanList[i];
		while (span) {
			//BlobPoint startPoint(span->start%screenWidth,span->start/screenWidth);
			pointList.push_back(BlobPoint(span->start%screenWidth,span->start/screenWidth));
			
			//BlobPoint endPoint(span->end%screenWidth,span->end/screenWidth);
			pointList.push_back(BlobPoint(span->end%screenWidth,span->end/screenWidth));
			
			span = span->next;
		}
		//BlobPoint emptyPoint(-1,-1);
		pointList.push_back(BlobPoint(-1,-1));
	}
	return pointList;
}

void BlobObject::computeOrientedBoundingBox() {
	
	unsigned int size = convexHull.size();
	double *a = new double[size];
	BlobPoint *m = new BlobPoint[size];
	BlobPoint *p1, *p2, mid;
	
	double area = 0.0;
	
	for (unsigned int i = 0; i < size; i++) {
		p1 = &convexHull[i];
		p2 = &convexHull[(i+1)%size];
		area += a[i] = p1->distance(p2);
		m[i] = BlobPoint(p1).add(p2)->scale(0.5);
		mid.add(BlobPoint(&m[i]).scale(a[i]));
	}
	
	mid.scale(1.0/area);
	area = 1.0/(4.0*area);
	
	double c[4];
	double sum;
	
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			sum = 0.0;
			
			for (unsigned int k = 0; k < size; k++) {
				p1 = &convexHull[k];
				p2 = &convexHull[(k+1)%size];
				sum += a[k]*(2.0*m[k].get(i)*m[k].get(j) + p1->get(i)*p1->get(j) + p2->get(i)*p2->get(j));
			}
			
			sum *= area;
			c[(i<<1)+j] = sum - mid.get(i)*mid.get(j);
		}
	}
	
	BlobMatrix C(c);
	double *lambda = C.eigenvalues();
	if (lambda==NULL) {
		delete[]a;
		delete[]m;
		return;
	}
	
	BlobMatrix X(1.0, 0.0, 0.0, 1.0);
	X.scale(lambda[0])->sub(&C);
	BlobMatrix Y(1.0, 0.0, 0.0, 1.0);
	Y.scale(lambda[1])->sub(&C);
	delete[] lambda;
	
	BlobPoint i1,i2;
	BlobPoint *s1 = X.solve(&i1);
	BlobPoint *s2 = Y.solve(&i2);
	
	obBox.clear();
	if ((s1==NULL) || (s2==NULL)) {
		delete[]a;
		delete[]m;
		return;
	}
	
	s1->normalize();
	s2->normalize();
	
	BlobMatrix BC;
	BC.setRow(0, s1);
	BC.setRow(1, s2);
	
	float minX = screenWidth*screenHeight;
	float maxX = -minX;
	float minY = screenWidth*screenHeight;
	float maxY = -minY;
	
	BlobPoint projected;
	for (unsigned int i = 0; i < size; i++) {
		BC.multiply(&convexHull[i], &projected);
		if (projected.x < minX) minX = projected.x;
		if (projected.x > maxX) maxX = projected.x;
		if (projected.y < minY) minY = projected.y;
		if (projected.y > maxY) maxY = projected.y;
	}
	
	BlobPoint obb[4];
	BC.transpose();
	
	projected.x = minX; projected.y = minY;
	BC.multiply(&projected, &obb[0]);
	
	projected.x = maxX; projected.y = minY;
	BC.multiply(&projected, &obb[1]);
	
	projected.x = maxX; projected.y = maxY;
	BC.multiply(&projected, &obb[2]);
	
	projected.x = minX; projected.y = maxY;
	BC.multiply(&projected, &obb[3]);
	
	float minx = screenWidth;	int minxi = 0;
	float maxx = 0;				int maxxi = 0;
	float miny = screenHeight;	int minyi = 0;
	float maxy = 0;				int maxyi = 0;
	
	for (int i=0;i<4;i++) {
		if (obb[i].x < minx) { minx = obb[i].x; minxi = i; }
		if (obb[i].x > maxx) { maxx = obb[i].x; maxxi = i; }
		if (obb[i].y < miny) { miny = obb[i].y; minyi = i; }
		if (obb[i].y > maxy) { maxy = obb[i].y; maxyi = i; }
	}
	
	if (minxi==minyi) {
		int left = 6-(minxi+maxxi+maxyi);
		if (obb[left].y==minyi) minyi=left;
		else minxi=left;
	} else if (maxxi==maxyi) {
		int left = 6-(maxxi+minxi+minyi);
		if (obb[left].y==maxy) maxyi=left;
		else maxxi=left;
	}
	
	// corners
	obBox.push_back(BlobPoint(minx,obb[minxi].y));
	obBox.push_back(BlobPoint(obb[minyi].x,miny));
	obBox.push_back(BlobPoint(maxx,obb[maxxi].y));
	obBox.push_back(BlobPoint(obb[maxyi].x,maxy));
	
	// center point
	obBox.push_back(BlobPoint(1+ maxx - (maxx - minx)/2.0f,1+ maxy - (maxy - miny)/2.0f));
	
	// width & height
	//float w = 1+sqrtf((obBox[0].x-obBox[1].x)*(obBox[0].x-obBox[1].x)+(obBox[0].y-obBox[1].y)*(obBox[0].y-obBox[1].y));
	//float h = 1+sqrtf((obBox[1].x-obBox[2].x)*(obBox[1].x-obBox[2].x)+(obBox[1].y-obBox[2].y)*(obBox[1].y-obBox[2].y));
	
	double w = obBox[1].distance(&obBox[0]);
	double h = obBox[1].distance(&obBox[2]);
	obBox.push_back(BlobPoint(w,h));
	
	delete[]a;
	delete[]m;
}
/*
double theta1(BlobPoint *p1, BlobPoint *p2) {
	double dx = p2->x - p1->x;
	double ax = fabs(dx);
	double dy = p2->y - p1->y;
	double ay = fabs(dy);
	double t  = ax+ay;
	
	t = t < 1e-8 ? 0.0 : dy/t;
	if (dx < 0.0) t = 2.0-t;
	else if (dy < 0.0) t += 4.0;
	return t*90.0;
}

void BlobObject::computeConvexHull() {
	
	convexHull = outerContour;
	
	unsigned int N = convexHull.size()-1;
	unsigned int i, min, M;
	double th, v, x;
	
	BlobPoint t;
	
	for (min = 0, i = 1; i < N; i++) {
		if (convexHull[i].y < convexHull[min].y)
			min = i;
	}
	
	convexHull[N] = convexHull[min];
	th = 0.0;
	
	for (M = 0; M < N; M++) {
		t = convexHull[M];
		convexHull[M] = convexHull[min];
		convexHull[min] = t;
		
		min = N;
		v = th;
		th = 360.0;
		
		t = convexHull[M];
		
		for (i = M+1; i <= N; i++) {
			x = theta1(&t, &convexHull[i]);
			if ((x > v) && (x <= th))  {
				min = i;
				th = x;
			}
		}
		if (min == N) break;
	}
	
	while(convexHull.size()>M+1) convexHull.pop_back();
	//std::cout << "convex hull: " << convexHull.size() << std::endl;
}
*/

double cross(const BlobPoint &O, const BlobPoint &A, const BlobPoint &B)
{
	return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

void BlobObject::computeConvexHull() {
	//https://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
	
	int size = outerContour.size(), k = 0;
	if (size == 1) return;
	convexHull.resize(2*size);
	
	// Build lower hull
	for (int i = 0; i < size; ++i) {
		while (k >= 2 && cross(convexHull[k-2], convexHull[k-1], outerContour[i]) <= 0) k--;
		convexHull[k++] = outerContour[i];
	}
	
	// Build upper hull
	for (int i = size-2, t = k+1; i >= 0; i--) {
		while (k >= t && cross(convexHull[k-2], convexHull[k-1], outerContour[i]) <= 0) k--;
		convexHull[k++] = outerContour[i];
	}
	
	convexHull.resize(k-1);
	//std::cout << "convex hull: " << convexHull.size() << std::endl;
}

