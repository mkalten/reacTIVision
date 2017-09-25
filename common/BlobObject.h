/*  reacTIVision tangible interaction framework
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

#ifndef INCLUDED_BLOBOBJECT_H
#define INCLUDED_BLOBOBJECT_H

#include <math.h>
#include "UserInterface.h"
#include "TuioBlob.h"
#include "BlobMatrix.h"
#include "floatpoint.h"
#include "fidtrackX.h"
#include <vector>
#include <iostream>
#include <algorithm>

namespace TUIO {

	/**
	 * The BlobObject extends TuioBlob adding a few internal data structures
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.6
	 */
	class BlobObject: public TuioBlob {

	public:
		
		BlobObject(TuioTime ttime, Region *region, ShortPoint *dmap, bool do_full_analysis=false);
		
		//void setX(float xp) { xpos = xp; }
		//void setY(float yp) { ypos = yp; }
		
		float getRawX() { return rawXpos; }
		float getRawY() { return rawYpos; }
		float getRawWidth() { return rawWidth; }
		float getRawHeight() { return rawHeight; }

		std::vector<BlobPoint> getOrientedBoundingBox() {
			return obBox;
		}

		std::vector<BlobPoint> getConvexHull() {
			return convexHull;
		}

		std::vector<BlobPoint> getOuterContour() {
			return outerContour;
		}

		std::vector<BlobPoint> getFullContour() {
			return fullContour;
		}

		std::vector<BlobSpan*> getSpanList() {
			return spanList;
		}
		
		int getColour() {
			return blobRegion->colour;
		}
		
		Region* getRegion() {
			return blobRegion;
		}
		
		static void setDimensions(int w, int h) {
			screenWidth = w;
			screenHeight = h;
		}
		
		static void setInterface(UserInterface *uiface) {
			ui = uiface;
		}
		
	private:
		
		std::vector<BlobPoint> obBox;
		std::vector<BlobPoint> convexHull;
		std::vector<BlobPoint> outerContour;
		std::vector<BlobPoint> fullContour;
		std::vector<BlobSpan*> spanList;
		
		Region *blobRegion;
		
		std::vector<Span*> innerSpanList;
		//std::list<Span*> sortedSpanList;
		std::vector<BlobSpan> fullSpanList;
		
		void computeSpanList();
		void computeFullContourList();
		void computeOuterContourList(bool do_full_analyis);
		void computeInnerSpanList();
		void computeOrientedBoundingBox();
		void computeConvexHull();

		std::vector<BlobPoint> getInnerContourList();
		//std::vector<BlobPoint> getFullContourList();
		//std::vector<BlobPoint> getOuterContourList(std::vector<BlobPoint> fullList, std::vector<BlobPoint> innerList);
				
		static int screenWidth, screenHeight;
		static UserInterface* ui;
		
		float rawXpos, rawYpos;
		float rawWidth, rawHeight;

	};
};
#endif
