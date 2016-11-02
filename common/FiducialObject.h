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

#ifndef INCLUDED_FIDUCIALOBJECT_H
#define INCLUDED_FIDUCIALOBJECT_H

#include <math.h>
#include "TuioObject.h"
#include "floatpoint.h"
#include "fidtrackX.h"
#include <string>
#include <sstream>

#define FIDUCIAL_FOUND  0
#define FIDUCIAL_FUZZY -1
#define FIDUCIAL_ROOT  -2
#define FIDUCIAL_LOST  -3

#define CONFLICT_MAX 9

namespace TUIO {
	
	/**
	 * The FiducialObject extends TuioObject adding a few fiducial specific attributes
	 */
	class FiducialObject: public TuioObject {
		
	protected:
		/**
		 * Determines if the fiducial was tracked correctly
		 */
		int tracking_state;
		/**
		 * The colour of the fiducial root node
		 */
		int root_colour;
		/**
		 * The size of the fiducial root node
		 */
		float root_size;
		/**
		 * The offset of the fiducial center from the root node center
		 */
		FloatPoint root_offset;
		
		unsigned int conflict_threshold;
		unsigned int conflict_counter;
		int conflict_id;
		
	public:
		/**
		 * This constructor takes a TuioTime argument and assigns it along with the provided
		 * Session ID, Symbol ID, X and Y coordinate and angle to the newly created FiducialObject.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	si	the Session ID  to assign
		 * @param	sym	the Symbol ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 */
		FiducialObject (TuioTime ttime, long si, int sym, float xp, float yp, float a):TuioObject(ttime, si, sym, xp, yp, a) {
			tracking_state = FIDUCIAL_FOUND;
			conflict_threshold = 2;
			conflict_counter   = 0;
			conflict_id = INVALID_FIDUCIAL_ID;
		};
		
		
		/**
		 */
		void setFiducialInfo (int rcolour, int rsize) {
			root_colour = rcolour;
			root_size   = rsize;
		};
		
		bool checkIdConflict(int c_id) {
			if (c_id == conflict_id) {
				conflict_counter++;
				conflict_threshold--;
				
				if (conflict_counter>conflict_threshold) {
					conflict_threshold = 2;
					conflict_counter   = 0;
					conflict_id = INVALID_FIDUCIAL_ID;
					return true;
				}
			} else {
				conflict_counter = 0;
				conflict_id   = c_id;
			}
			return false;
		}
		
		/**
		 */
		void setRootColour (int colour) {
			root_colour = colour;
		};
		
		/**
		 */
		int getRootColour () {
			return root_colour;
		};
		
		/**
		 */
		void setRootSize (float size) {
			root_size = size;
		};
		
		/**
		 */
		float getRootSize () {
			return root_size;
		};
		
		/**
		 */
		void setRootOffset (float x, float y) {
			root_offset.x = x;
			root_offset.y = y;
		};
		
		/**
		 */
		FloatPoint getRootOffset () {
			return root_offset;
		};
		
		/**
		 */
		void setTrackingState (int track) {
			tracking_state = track;
			if ((tracking_state==FIDUCIAL_FOUND) && (conflict_threshold<CONFLICT_MAX)) conflict_threshold++;
			//else conflict_threshold--;
		};
		
		/**
		 */
		int getTrackingState() {
			return tracking_state;
		};
		
		/**
		 */
		std::string getStatistics() {
			std::stringstream message;
			message << "obj ";
			switch(tracking_state) {
				case FIDUCIAL_FOUND:
					message << "FND ";
					break;
				case FIDUCIAL_LOST:
					message << "LST ";
					break;
				case FIDUCIAL_FUZZY:
					message << "FUZ ";
					break;
				case FIDUCIAL_ROOT:
					message << "BLB ";
					break;
				default:
					message << "UND ";
					break;
			}
			message << session_id <<" "<< symbol_id <<" "<<  xpos <<" "<<  ypos <<" "<<  angle;
			
			return message.str();
		};
	};
};
#endif
