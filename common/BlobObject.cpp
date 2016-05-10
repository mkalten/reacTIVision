/*	reacTIVision tangible interaction framework
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

#include "BlobObject.h"
using namespace TUIO;

int BlobObject::screenWidth = WIDTH;
int BlobObject::screenHeight = HEIGHT;
UserInterface* BlobObject::ui = NULL;

BlobObject::BlobObject(TuioTime ttime, RegionX *region, ShortPoint *dmap, bool do_full_analyis):TuioBlob(ttime, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f) {
	
	
	if (region==NULL) throw std::exception();
	blobRegion = region;
	
	/*#ifndef NDEBUG
	 //fill the region candidate with yellow
	 Span *span = region->span;
	 iface->setColor(255, 255, 0);
	 while (span) {
		iface->drawLine(span->start%screenWidth,span->start/screenWidth,span->end%width,span->end/screenWidth);
		span = span->next;
	 }
	 
	 //draw a blue rectangle around the region candidate
	 iface->setColor(0,0,255);
	 iface->drawRect(region->left,region->top,region->right-region->left,region->bottom-region->top);
	 #endif*/
	
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
		ui->drawLine(orientedBoundingBox[i].x,orientedBoundingBox[i].y,orientedBoundingBox[j].x, orientedBoundingBox[j].y);
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
	
	rawWidth = obBox[5].x;
	rawHeight = obBox[5].y;
	
	width = rawWidth/screenWidth;
	height = rawHeight/screenHeight;
	
	area = (float)region->area/(screenWidth*screenHeight);
	
	float ak = obBox[1].x - obBox[0].x;
	float hp  = obBox[1].distance(&obBox[0]);
	angle = acosf(ak/hp);
	if (height>width) {
		width = rawHeight/screenWidth;
		height = rawWidth/screenHeight;
		angle+=3*M_PI/2.0f;
	}
	angle=2*M_PI-angle;
	if (angle<0) angle = 0;	
}

/*
 std::vector<BlobPoint> BlobObject::getOuterContourList(RegionX *region) {
 
	std::vector<BlobPoint> pointList;
	Span *span = region->span;
	fullSpanList.clear();
	while (span) {
 
 bool add_start = true;
 bool add_end = true;
 for (int i=0;i<region->inner_span_count;i++) {
 Span *inner = region->inner_spans[i];
 while (inner) {
 if (span->end+1==inner->start) {
 add_end=false;
 if(!add_start) i=region->inner_span_count;
 break;
 } else if (span->start-1==inner->end) {
 add_start=false;
 if(!add_end) i=region->inner_span_count;
 break;
 }
 inner = inner->next;
 }
 }
 
 if(add_start) {
 BlobPoint startPoint(span->start%screenWidth, span->start/screenWidth);
 pointList.push_back(startPoint);
 }
 
 if (add_end) {
 BlobPoint endPoint(span->end%screenWidth, span->end/screenWidth);
 pointList.push_back(endPoint);
 }
 
 span = span->next;
	}
	return pointList;
 }
 */



void BlobObject::computeOuterContourList() {
	
	outerContour.clear();
	std::vector<BlobPoint> reverseList;
	
	Span *span = blobRegion->span;
	fullSpanList.clear();
	while (span) {
		
		bool add_start = true;
		bool add_end = true;
		for (int i=0;i<blobRegion->inner_span_count;i++) {
			Span *inner = blobRegion->inner_spans[i];
			while (inner) {
				if (span->end+1==inner->start) {
					add_end=false;
					if(!add_start) i=blobRegion->inner_span_count;
					break;
				} else if (span->start-1==inner->end) {
					add_start=false;
					if(!add_end) i=blobRegion->inner_span_count;
					break;
				}
				inner = inner->next;
			}
		}
		
		if(add_start) {
			BlobPoint startPoint(span->start%screenWidth, span->start/screenWidth);
			outerContour.push_back(startPoint);
		}
		
		if (add_end) {
			BlobPoint endPoint(span->end%screenWidth, span->end/screenWidth);
			reverseList.insert(reverseList.begin(),endPoint);
		}
		
		span = span->next;
	}
	
	outerContour.insert( outerContour.end(), reverseList.begin(), reverseList.end() );
}

void BlobObject::computeFullContourList() {
	
	fullContour.clear();
	
	for (unsigned int row=0;row<spanList.size();row++) {
		
		BlobSpan *row_span = &spanList[row];
		
		
		if ((row==0) || (row==spanList.size()-1)) {
			
			while (row_span) {
				
				for (int i=row_span->start;i<=row_span->end;i++) {
					BlobPoint spanPoint(i%screenWidth, i/screenWidth);
					fullContour.push_back(spanPoint);
					
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
	
	/*std::list<Span*> currentSpans;
	 std::list<Span*> upperSpans;
	 int last_y = (*spanList.begin())->start/screenWidth;
	 
	 std::vector<BlobPoint> pointList;
	 for (std::list<Span*>::iterator span = spanList.begin(); span != spanList.end(); span++) {
		
		BlobSpan nspan = { (*span)->start, (*span)->end };
		fullSpanList.push_back(nspan);
		
		BlobPoint startPoint;
		startPoint.x = (*span)->start%screenWidth;
		startPoint.y = (*span)->start/screenWidth;
		pointList.push_back(startPoint);
		
		ui->setColor(0,0,255);
		ui->drawPoint(startPoint.x,startPoint.y);
		
		BlobPoint endPoint;
		endPoint.x = (*span)->end%screenWidth;
		endPoint.y = (*span)->end/screenWidth;
		
		if (startPoint.y>last_y) {
	 
	 //upperSpans.clear();
	 //for (std::list<Span*>::iterator cspan = currentSpans.begin(); cspan != currentSpans.end(); cspan++) {
	 //	upperSpans.push_back(*cspan);
	 //}
	 //currentSpans.clear();
	 upperSpans = currentSpans;
	 currentSpans.clear();
	 last_y = startPoint.y;
	 
	 //printf("spans: %ld\n",upperSpans.size());
		}
		
		currentSpans.push_back((*span));
	 
		
		if (upperSpans.size()>0) {
	 
	 int start = (*span)->start+1;
	 for (std::list<Span*>::iterator uspan = upperSpans.begin(); uspan != upperSpans.end(); uspan++) {
	 
	 int end = (*uspan)->start+screenWidth;
	 
	 for (int pos=start;pos<end;pos++) {
	 BlobPoint fillPoint;
	 fillPoint.x = pos%screenWidth;
	 if ((fillPoint.x>startPoint.x) && (fillPoint.x<endPoint.x)) {
	 fillPoint.y = pos/screenWidth;
	 pointList.push_back(fillPoint);
	 
	 ui->setColor(255,0,0);
	 ui->drawPoint(fillPoint.x,fillPoint.y);
	 }
	 }
	 
	 start = (*uspan)->end+screenWidth+1;
	 }
	 
	 int end = (*span)->end;
	 for (int pos=start;pos<end;pos++) {
	 BlobPoint fillPoint;
	 fillPoint.x = pos%screenWidth;
	 if ((fillPoint.x>startPoint.x) && (fillPoint.x<endPoint.x)) {
	 fillPoint.y = pos/screenWidth;
	 pointList.push_back(fillPoint);
	 
	 ui->setColor(255,0,0);
	 ui->drawPoint(fillPoint.x,fillPoint.y);
	 }
	 }
	 
		} else {
	 
	 int start = (*span)->start+1;
	 int end = (*span)->end;
	 
	 for (int pos=start;pos<end;pos++) {
	 BlobPoint fillPoint;
	 fillPoint.x = pos%screenWidth;
	 fillPoint.y = pos/screenWidth;
	 pointList.push_back(fillPoint);
	 
	 ui->setColor(255,0,0);
	 ui->drawPoint(fillPoint.x,fillPoint.y);
	 }
		}
		
	 
		pointList.push_back(endPoint);
		
		ui->setColor(0,0,255);
		ui->drawPoint(endPoint.x,endPoint.y);
	 }*/
	
}

void BlobObject::computeSpanList() {
	
	spanList.clear();
	fullSpanList.clear();
	sortedSpanList.clear();
	
	Span *span = blobRegion->span;
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


std::vector<BlobPoint> BlobObject::getInnerContourList(RegionX *region) {
	std::vector<BlobPoint> pointList;
	
	for (int i=0;i<region->inner_span_count;i++) {
		Span *span = region->inner_spans[i];
		//std::vector<Span*> spanList;
		while (span) {
			BlobPoint startPoint;
			startPoint.x = span->start%screenWidth;
			startPoint.y = span->start/screenWidth;
			pointList.push_back(startPoint);
			
			BlobPoint endPoint;
			endPoint.x = span->end%screenWidth;
			endPoint.y = span->end/screenWidth;
			pointList.push_back(endPoint);
			
			span = span->next;
		}
		BlobPoint emptyPoint;
		emptyPoint.x = -1;
		emptyPoint.y = -1;
		pointList.push_back(emptyPoint);
	}
	return pointList;
}

/*
 std::vector<BlobPoint> BlobObject::getOuterContourList(std::vector<BlobPoint> fullList, std::vector<BlobPoint> innerList) {
 
	std::vector<BlobPoint> pointList;
	for (std::vector<BlobPoint>::iterator full = fullList.begin(); full != fullList.end(); full++) {
 std::vector<BlobPoint>::iterator inner;
 for (inner = innerList.begin(); inner != innerList.end(); inner++) {
 if (((full->x==inner->x-1) || (full->x==inner->x+1)) && (full->y==inner->y))  break;
 }
 if (inner==innerList.end()) pointList.push_back(*full);
	}
	return pointList;
 }
 */

void BlobObject::computeOrientedBoundingBox() {
	
	double *a = new double[convexHull.size()];
	BlobPoint *m = new BlobPoint[convexHull.size()];
	BlobPoint p1, p2, mid;
	
	double area = 0.0;
	
	for (unsigned int i = 0; i < convexHull.size(); i++) {
		p1 = convexHull[i];
		p2 = convexHull[(i+1)%convexHull.size()];
		area += a[i] = p1.distance(&p2);
		m[i] = BlobPoint(&p1).add(&p2)->scale(0.5);
		mid.add( BlobPoint(&m[i]).scale(a[i]));
	}
	
	mid.scale(1.0/area);
	area = 1.0/(4.0*area);
	
	double c[4];
	double sum;
	
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			sum = 0.0;
			
			for (unsigned int k = 0; k < convexHull.size(); k++) {
				p1 = convexHull[k];
				p2 = convexHull[(k+1)%convexHull.size()];
				sum += a[k]*(2.0*m[k].get(i)*m[k].get(j) + p1.get(i)*p1.get(j) + p2.get(i)*p2.get(j));
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
	
	double minX = screenWidth*screenHeight;
	double maxX = -minX;
	double minY = screenWidth*screenHeight;
	double maxY = -minY;
	
	BlobPoint projected;
	for (unsigned int i = 0; i < convexHull.size(); i++) {
		BC.multiply(&convexHull[i], &projected);
		if (projected.x < minX) minX = projected.x;
		if (projected.x > maxX) maxX = projected.x;
		if (projected.y < minY) minY = projected.y;
		if (projected.y > maxY) maxY = projected.y;
	}
	
	BC.transpose();
	BlobPoint obb[4];
	
	BlobPoint bp0;
	projected.x = minX; projected.y = minY;
	obb[0] = BC.multiply(&projected,&bp0);
	
	BlobPoint bp1;
	projected.x = maxX; projected.y = minY;
	obb[1] = BC.multiply(&projected, &bp1);
	
	BlobPoint bp2;
	projected.x = maxX; projected.y = maxY;
	obb[2] = BC.multiply(&projected, &bp2);
	
	BlobPoint bp3;
	projected.x = minX; projected.y = maxY;
	obb[3] = BC.multiply(&projected, &bp3);
	
	double minx = screenWidth;  int minxi = 0;
	double maxx = 0;			int maxxi = 0;
	double miny = screenHeight; int minyi = 0;
	double maxy = 0;			int maxyi = 0;
	
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
	double w = 1+sqrt((obBox[0].x-obBox[1].x)*(obBox[0].x-obBox[1].x)+(obBox[0].y-obBox[1].y)*(obBox[0].y-obBox[1].y));
	double h = 1+sqrt((obBox[1].x-obBox[2].x)*(obBox[1].x-obBox[2].x)+(obBox[1].y-obBox[2].y)*(obBox[1].y-obBox[2].y));
	obBox.push_back(BlobPoint(w,h));
	
	delete[]a;
	delete[]m;
}


double BlobObject::theta1(BlobPoint *p1, BlobPoint *p2) {
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
}


