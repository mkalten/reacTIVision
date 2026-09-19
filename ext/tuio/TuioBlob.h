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

#ifndef INCLUDED_TUIOBLOB_H
#define INCLUDED_TUIOBLOB_H

#include "TuioContainer.h"

namespace TUIO {
	
	/**
	 * The TuioBlob class encapsulates /tuio/2Dblb TUIO blobs.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL TuioBlob: public TuioContainer {
		
	protected:
		/**
		 * The individual blob ID number that is assigned to each TuioBlob.
		 */ 
		int blob_id;
		/**
		 * The rotation angle value.
		 */ 
		float angle;
		/**
		 * The width value.
		 */ 
		float width;
		/**
		 * The height value.
		 */ 
		float height;
		/**
		 * The area value.
		 */ 
		float area;
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
		/**
		 * Optional size threshold suppressing width and height updates below this value
		 */ 
		float sizeThreshold;
		/**
		 * Optional OneEuroFilter for the width smoothing
		 */ 
		OneEuroFilter *widthFilter;
		/**
		 * Optional OneEuroFilter for the height smoothing
		 */ 
		OneEuroFilter *heightFilter;
		
	public:
		using TuioContainer::update;

		/**
		 * This constructor takes a TuioTime argument and assigns it along with the provided 
		 * Session ID, X and Y coordinate, width, height and angle to the newly created TuioBlob.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	si	the Session ID  to assign
		 * @param	bi	the Blob ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 */
		TuioBlob (TuioTime ttime, int si, int bi, float xp, float yp, float a, float w, float h, float f);

		/**
		 * This constructor takes the provided Session ID, X and Y coordinate,
		 * angle, width, height and area, and assigns these values to the newly created TuioBlob.
		 *
		 * @param	si	the Session ID  to assign
		 * @param	bi	the Blob ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 */	
		TuioBlob (int si, int bi, float xp, float yp, float a, float  w, float h, float f);
		
		/**
		 * This constructor takes the attributes of the provided TuioBlob 
		 * and assigns these values to the newly created TuioBlob.
		 *
		 * @param	tblb	the TuioBlob to assign
		 */
		TuioBlob (TuioBlob *tblb);

		/**
		 * The copy constructor copies all attributes of the provided TuioBlob,
		 * including the path and a deep copy of the optional filters.
		 *
		 * @param	tblb	the TuioBlob to copy
		 */
		TuioBlob (const TuioBlob &tblb);

		/**
		 * The assignment operator copies all attributes of the provided TuioBlob,
		 * including the path and a deep copy of the optional filters.
		 *
		 * @param	tblb	the TuioBlob to copy
		 * @return	a reference to this TuioBlob
		 */
		TuioBlob& operator=(const TuioBlob &tblb);
		
		/**
		 * The destructor is doing nothing in particular. 
		 */
		virtual ~TuioBlob() {
			if (widthFilter) delete widthFilter;
			if (heightFilter) delete heightFilter;
			if (angleFilter) delete angleFilter;
		};

		/**
		 * Returns the Blob ID of this TuioBlob.
		 * @return	the Blob ID of this TuioBlob
		 */
		int getBlobID() const;
		
		/**
		 * Sets the Blob ID of this TuioBlob.
		 * @param bi	the new Blob ID for this TuioBlob
		 */
		void setBlobID(int bi);
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate, angle, width, height, area, X and Y velocity, motion acceleration,
		 * rotation speed and rotation acceleration to the private TuioBlob attributes.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the rotation angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	rs	the rotation velocity to assign
		 * @param	ma	the motion acceleration to assign
		 * @param	ra	the rotation acceleration to assign
		 */
		void update (TuioTime ttime, float xp, float yp, float a, float w, float h, float f, float xs, float ys, float rs, float ma, float ra);

		/**
		 * Assigns the provided X and Y coordinate, angle, width, height, area, X and Y velocity,
		 * motion acceleration, rotation velocity and rotation acceleration to the private TuioBlob attributes.
		 * The TuioTime time stamp remains unchanged.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	rs	the rotation velocity to assign
		 * @param	ma	the motion acceleration to assign
		 * @param	ra	the rotation acceleration to assign
		 */
		void update (float xp, float yp, float a, float w, float h, float f, float xs, float ys, float rs, float ma, float ra);
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate, angle, width, height and area to the private TuioBlob attributes.
		 * The speed and acceleration values are calculated accordingly.
		 * The angle value is processed by the optional angle filter and angle threshold,
		 * the width and height values by the optional size filter and size threshold.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @param	w	the width to assign
		 * @param	h	the height to assign
		 * @param	f	the area to assign
		 */
		void update (TuioTime ttime, float xp, float yp, float a, float w, float h, float f);

		/**
		 * This method is used to calculate the speed and acceleration values of a
		 * TuioBlob with unchanged position and angle.
		 */
		void stop (TuioTime ttime);
		
		/**
		 * Takes the attributes of the provided TuioBlob 
		 * and assigns these values to this TuioBlob.
		 * The TuioTime time stamp of this TuioBlob remains unchanged.
		 *
		 * @param	tblb	the TuioBlob to assign
		 */	
		void update (TuioBlob *tblb);
		
		/**
		 * Returns the width of this TuioBlob.
		 * @return	the width of this TuioBlob
		 */
		float getWidth() const;

		/**
		 * Returns the height of this TuioBlob.
		 * @return	the height of this TuioBlob
		 */
		float getHeight() const;

		/**
		 * Returns the width of this TuioBlob in pixels relative to the provided screen width.
		 *
		 * @param	w	the screen width
		 * @return	the width of this TuioBlob in pixels
		 */
		int getScreenWidth(int w) const;
		
		/**
		 * Returns the height of this TuioBlob in pixels relative to the provided screen height.
		 *
		 * @param	h	the screen height
		 * @return	the height of this TuioBlob in pixels
		 */
		int getScreenHeight(int h) const;
		
		/**
		 * Returns the area of this TuioBlob.
		 * @return	the area of this TuioBlob
		 */
		float getArea() const;
		
		/**
		 * Returns the rotation angle of this TuioBlob.
		 * @return	the rotation angle of this TuioBlob
		 */
		float getAngle() const;
		
		/**
		 * Returns the rotation angle in degrees of this TuioBlob.
		 * @return	the rotation angle in degrees of this TuioBlob
		 */
		float getAngleDegrees() const;
		
		/**
		 * Returns the rotation speed of this TuioBlob.
		 * @return	the rotation speed of this TuioBlob
		 */
		float getRotationSpeed() const;
		
		/**
		 * Returns the rotation acceleration of this TuioBlob.
		 * @return	the rotation acceleration of this TuioBlob
		 */
		float getRotationAccel() const;

		/**
		 * Returns true if this TuioBlob is moving.
		 * @return	true if this TuioBlob is moving
		 */
		bool isMoving() const;
		
		/**
		 * Adds a rotation angle threshold to this TuioBlob. Angle updates below
		 * the provided threshold are filtered out.
		 *
		 * @param	thresh	the angle threshold to apply
		 */
		void addAngleThreshold(float thresh);
		
		/**
		 * Removes the rotation angle threshold from this TuioBlob.
		 */
		void removeAngleThreshold();
		
		/**
		 * Adds a OneEuroFilter to the rotation angle of this TuioBlob,
		 * smoothing the rotation updates of the update(TuioTime,float,float,float,float,float,float) method.
		 *
		 * @param	mcut	the minimum cutoff frequency, must be > 0
		 * @param	beta	the cutoff slope, must be > 0
		 */
		void addAngleFilter(float mcut, float beta);
		
		/**
		 * Removes the OneEuroFilter from the rotation angle of this TuioBlob.
		 */
		void removeAngleFilter();
		
		/**
		 * Adds a size threshold to this TuioBlob. Width and height updates below
		 * the provided threshold are filtered out.
		 *
		 * @param	thresh	the size threshold to apply
		 */
		void addSizeThreshold(float thresh);
		
		/**
		 * Removes the size threshold from this TuioBlob.
		 */
		void removeSizeThreshold();
		
		/**
		 * Adds a OneEuroFilter to the width and height of this TuioBlob,
		 * smoothing the size updates of the update(TuioTime,float,float,float,float,float,float) method.
		 *
		 * @param	mcut	the minimum cutoff frequency, must be > 0
		 * @param	beta	the cutoff slope, must be > 0
		 */
		void addSizeFilter(float mcut, float beta);
		
		/**
		 * Removes the OneEuroFilter from the width and height of this TuioBlob.
		 */
		void removeSizeFilter();
	};
}
#endif
