/*
 TUIO C++ Library
 Copyright (c) 2005-2017 Martin Kaltenbrunner <martin@tuio.org>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3.0 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library.
*/

#ifndef INCLUDED_TUIOOBJECT_H
#define INCLUDED_TUIOOBJECT_H

#include "TuioContainer.h"

namespace TUIO {
	
	/**
	 * The TuioObject class encapsulates /tuio/2Dobj TUIO objects.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL TuioObject: public TuioContainer {
		
	protected:
		/**
		 * The individual symbol ID number that is assigned to each TuioObject.
		 */ 
		int symbol_id;
		/**
		 * The rotation angle value.
		 */ 
		float angle;
		/**
		 * The rotation speed value.
		 */ 
		float rotation_speed;
		/**
		 * The rotation acceleration value.
		 */ 
		float rotation_accel;
		
		/**
		 * Optional angle threshold suppressing rotation updates below this angle
		 */ 
		float angleThreshold;
		/**
		 * Optional OneEuroFilter for the rotation angle smoothing
		 */ 
		OneEuroFilter *angleFilter;
		
	public:
		using TuioContainer::update;
		
		/**
		 * This constructor takes a TuioTime argument and assigns it along with the provided 
		 * Session ID, Symbol ID, X and Y coordinate and angle to the newly created TuioObject.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	si	the Session ID  to assign
		 * @param	sym	the Symbol ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 */
		TuioObject (TuioTime ttime, int si, int sym, float xp, float yp, float a);

		/**
		 * This constructor takes the provided Session ID, Symbol ID, X and Y coordinate 
		 * and angle, and assigns these values to the newly created TuioObject.
		 *
		 * @param	si	the Session ID  to assign
		 * @param	sym	the Symbol ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 */	
		TuioObject (int si, int sym, float xp, float yp, float a);
		
		/**
		 * This constructor takes the attributes of the provided TuioObject 
		 * and assigns these values to the newly created TuioObject.
		 *
		 * @param	tobj	the TuioObject to assign
		 */
		TuioObject (TuioObject *tobj);

		/**
		 * The copy constructor copies all attributes of the provided TuioObject,
		 * including the path and a deep copy of the optional filters.
		 *
		 * @param	tobj	the TuioObject to copy
		 */
		TuioObject (const TuioObject &tobj);

		/**
		 * The assignment operator copies all attributes of the provided TuioObject,
		 * including the path and a deep copy of the optional filters.
		 *
		 * @param	tobj	the TuioObject to copy
		 * @return	a reference to this TuioObject
		 */
		TuioObject& operator=(const TuioObject &tobj);
		
		/**
		 * The destructor is doing nothing in particular. 
		 */
		virtual ~TuioObject() {
			if (angleFilter) delete angleFilter;
		};
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate, angle, X and Y velocity, motion acceleration,
		 * rotation speed and rotation acceleration to the private TuioObject attributes.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	rs	the rotation velocity to assign
		 * @param	ma	the motion acceleration to assign
		 * @param	ra	the rotation acceleration to assign
		 */
		void update (TuioTime ttime, float xp, float yp, float a, float xs, float ys, float rs, float ma, float ra);

		/**
		 * Assigns the provided X and Y coordinate, angle, X and Y velocity, motion acceleration
		 * rotation velocity and rotation acceleration to the private TuioContainer attributes.
		 * The TuioTime time stamp remains unchanged.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	rs	the rotation velocity to assign
		 * @param	ma	the motion acceleration to assign
		 * @param	ra	the rotation acceleration to assign
		 */
		void update (float xp, float yp, float a, float xs, float ys, float rs, float ma, float ra);
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate and angle to the private TuioObject attributes.
		 * The speed and acceleration values are calculated accordingly.
		 * The angle value is processed by the optional angle filter and angle threshold.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 */
		void update (TuioTime ttime, float xp, float yp, float a);

		/**
		 * This method is used to calculate the speed and acceleration values of a
		 * TuioObject with unchanged position and angle.
		 */
		void stop (TuioTime ttime);
		
		/**
		 * Takes the attributes of the provided TuioObject 
		 * and assigns these values to this TuioObject.
		 * The TuioTime time stamp of this TuioObject remains unchanged.
		 *
		 * @param	tobj	the TuioObject to assign
		 */	
		void update (TuioObject *tobj);
		
		/**
		 * Returns the symbol ID of this TuioObject.
		 * @return	the symbol ID of this TuioObject
		 */
		int getSymbolID() const;

		/**
		 * Sets the symbol ID of this TuioObject.
		 * @param sym	the new symbol ID for this TuioObject
		 */
		void setSymbolID(int sym);
		
		/**
		 * Returns the rotation angle of this TuioObject.
		 * @return	the rotation angle of this TuioObject
		 */
		float getAngle() const;
		
		/**
		 * Returns the rotation angle in degrees of this TuioObject.
		 * @return	the rotation angle in degrees of this TuioObject
		 */
		float getAngleDegrees() const;
		
		/**
		 * Returns the rotation speed of this TuioObject.
		 * @return	the rotation speed of this TuioObject
		 */
		float getRotationSpeed() const;
		
		/**
		 * Returns the rotation acceleration of this TuioObject.
		 * @return	the rotation acceleration of this TuioObject
		 */
		float getRotationAccel() const;

		/**
		 * Returns true if this TuioObject is moving.
		 * @return	true if this TuioObject is moving
		 */
		bool isMoving() const;
		
		/**
		 * Adds a rotation angle threshold to this TuioObject. Angle updates below
		 * the provided threshold are filtered out.
		 *
		 * @param	thresh	the angle threshold to apply
		 */
		void addAngleThreshold(float thresh);
		
		/**
		 * Removes the rotation angle threshold from this TuioObject.
		 */
		void removeAngleThreshold();
		
		/**
		 * Adds a OneEuroFilter to the rotation angle of this TuioObject,
		 * smoothing the rotation updates of the update(TuioTime,float,float,float) method.
		 *
		 * @param	mcut	the minimum cutoff frequency, must be > 0
		 * @param	beta	the cutoff slope, must be > 0
		 */
		void addAngleFilter(float mcut, float beta);
		
		/**
		 * Removes the OneEuroFilter from the rotation angle of this TuioObject.
		 */
		void removeAngleFilter();
	};
}
#endif
