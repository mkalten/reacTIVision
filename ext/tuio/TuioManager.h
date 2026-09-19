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

#ifndef INCLUDED_TUIOMANAGER_H
#define INCLUDED_TUIOMANAGER_H

#include "TuioDispatcher.h"

#include <iostream>
#include <list>
#include <algorithm>

#define OBJ_MESSAGE_SIZE 108	// setMessage + fseqMessage size
#define CUR_MESSAGE_SIZE 88
#define BLB_MESSAGE_SIZE 116

namespace TUIO {
	/**
	 * <p>The TuioManager class is the central TUIO session management component.</p> 
	 * <p>During runtime each frame is marked with the initFrame and commitFrame methods, 
	 * while the currently present TuioObjects, TuioCursors and TuioBlobs are managed by the server
	 * with ADD, UPDATE and REMOVE methods in analogy to the TuioClient's TuioListener interface.</p> 
	 * <p><code>
	 * TuioManager *manager = new TuioManager();<br/>
	 * ...<br/>
	 * manager->initFrame(TuioTime::getSessionTime());<br/>
	 * TuioObject *tobj = manager->addTuioObject(symbol,xpos,ypos,angle);<br/>
	 * TuioCursor *tcur = manager->addTuioCursor(xpos,ypos);<br/>
	 * TuioBlob *tblb = manager->addTuioBlob(xpos,ypos,angle,width,height,area);<br/>
	 * manager->commitFrame();<br/>
	 * ...<br/>
	 * manager->initFrame(TuioTime::getSessionTime());<br/>
	 * manager->updateTuioObject(tobj,xpos,ypos,angle);<br/>
	 * manager->updateTuioCursor(tcur,xpos,ypos);<br/>
	 * manager->updateTuioBlob(tblb,xpos,ypos,angle,width,height,area);<br/>
	 * manager->commitFrame();<br/>
	 * ...<br/>
	 * manager->initFrame(TuioTime::getSessionTime());<br/>
	 * manager->removeTuioObject(tobj);<br/>
	 * manager->removeTuioCursor(tcur);<br/>
	 * manager->removeTuioBlob(tblb);<br/>
	 * manager->commitFrame();<br/>
	 * </code></p>
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL TuioManager : public TuioDispatcher { 
	
	public:

		/**
		 * The default constructor creates a TuioManager
		 */
		TuioManager();

		/**
		 * The destructor is doing nothing in particular. 
		 */
		~TuioManager();
		
		/**
		 * Creates a new TuioObject based on the given arguments.
		 * The new TuioObject is added to the TuioServer's internal list of active TuioObjects 
		 * and a reference is returned to the caller.
		 *
		 * @param	sym	the Symbol ID  to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 * @return	reference to the created TuioObject
		 */
		TuioObject* addTuioObject(int sym, float xp, float yp, float a);

		/**
		 * Updates the referenced TuioObject based on the given arguments.
		 *
		 * @param	tobj	the TuioObject to update
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	a	the angle to assign
		 */
		void updateTuioObject(TuioObject *tobj, float xp, float yp, float a);

		/**
		 * Removes the referenced TuioObject from the TuioServer's internal list of TuioObjects
		 * and deletes the referenced TuioObject afterwards
		 *
		 * @param	tobj	the TuioObject to remove
		 */
		void removeTuioObject(TuioObject *tobj);

		/**
		 * Adds an externally managed TuioObject to the TuioServer's internal list of active TuioObjects 
		 *
		 * @param	tobj	the TuioObject to add
		 */
		void addExternalTuioObject(TuioObject *tobj);

		/**
		 * Updates an externally managed TuioObject 
		 *
		 * @param	tobj	the TuioObject to update
		 */
		void updateExternalTuioObject(TuioObject *tobj);

		/**
		 * Removes an externally managed TuioObject from the TuioServer's internal list of TuioObjects
		 * The referenced TuioObject is not deleted
		 *
		 * @param	tobj	the TuioObject to remove
		 */
		void removeExternalTuioObject(TuioObject *tobj);
		
		/**
		 * Creates a new TuioCursor based on the given arguments.
		 * The new TuioCursor is added to the TuioServer's internal list of active TuioCursors 
		 * and a reference is returned to the caller.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @return	reference to the created TuioCursor
		 */
		TuioCursor* addTuioCursor(float xp, float yp);

		/**
		 * Updates the referenced TuioCursor based on the given arguments.
		 *
		 * @param	tcur	the TuioCursor to update
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		void updateTuioCursor(TuioCursor *tcur, float xp, float yp);

		/**
		 * Removes the referenced TuioCursor from the TuioServer's internal list of TuioCursors
		 * and deletes the referenced TuioCursor afterwards
		 *
		 * @param	tcur	the TuioCursor to remove
		 */
		void removeTuioCursor(TuioCursor *tcur);

		/**
		 * Adds an externally managed TuioCursor 
		 *
		 * @param	tcur	the TuioCursor to add
		 */
		void addExternalTuioCursor(TuioCursor *tcur);

		/**
		 * Updates an externally managed TuioCursor 
		 *
		 * @param	tcur	the TuioCursor to update
		 */
		void updateExternalTuioCursor(TuioCursor *tcur);

		/**
		 * Removes an externally managed TuioCursor from the TuioServer's internal list of TuioCursor
		 * The referenced TuioCursor is not deleted
		 *
		 * @param	tcur	the TuioCursor to remove
		 */
		void removeExternalTuioCursor(TuioCursor *tcur);

		/**
		 * Creates a new TuioBlob based on the given arguments.
		 * The new TuioBlob is added to the TuioServer's internal list of active TuioBlobs 
		 * and a reference is returned to the caller.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	angle	the angle to assign
		 * @param	width	the width to assign
		 * @param	height	the height to assign
		 * @param	area	the area to assign
		 * @return	reference to the created TuioBlob
		 */
		TuioBlob* addTuioBlob(float xp, float yp, float angle, float width, float height, float area);
		
		/**
		 * Updates the referenced TuioBlob based on the given arguments.
		 *
		 * @param	tblb	the TuioBlob to update
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	angle	the angle to assign
		 * @param	width	the width to assign
		 * @param	height	the height to assign
		 * @param	area	the area to assign
		 */
		void updateTuioBlob(TuioBlob *tblb, float xp, float yp, float angle, float width, float height, float area);
		
		/**
		 * Removes the referenced TuioBlob from the TuioServer's internal list of TuioBlobs
		 * and deletes the referenced TuioBlob afterwards
		 *
		 * @param	tblb	the TuioBlob to remove
		 */
		void removeTuioBlob(TuioBlob *tblb);
		
		/**
		 * Adds an externally managed TuioBlob to the TuioServer's internal list of active TuioBlobs
		 *
		 * @param	tblb	the TuioBlob to add
		 */
		void addExternalTuioBlob(TuioBlob *tblb);
		
		/**
		 * Updates an externally managed TuioBlob 
		 *
		 * @param	tblb	the TuioBlob to update
		 */
		void updateExternalTuioBlob(TuioBlob *tblb);
		
		/**
		 * Removes an externally managed TuioBlob from the TuioServer's internal list of TuioBlob
		 * The referenced TuioBlob is not deleted
		 *
		 * @param	tblb	the TuioBlob to remove
		 */
		void removeExternalTuioBlob(TuioBlob *tblb);		
		
		/**
		 * Initializes a new frame with the given TuioTime
		 *
		 * @param	ttime	the frame time
		 */
		void initFrame(TuioTime ttime);
		
		/**
		 * Commits the current frame.
		 * Generates and sends TUIO messages of all currently active and updated TuioObjects, TuioCursors and TuioBlobs.
		 */
		void commitFrame();

		/**
		 * Returns the next available Session ID for external use.
		 * @return	the next available Session ID for external use
		 */
		int getSessionID();

		/**
		 * Returns the current frame ID for external use.
		 * @return	the current frame ID for external use
		 */
		int getFrameID();
		
		/**
		 * Returns the current frame time for external use.
		 * @return	the current frame time for external use
		 */
		TuioTime getFrameTime();
		
		/**
		 * Returns a List of all currently inactive TuioObjects
		 *
		 * @return  a List of all currently inactive TuioObjects
		 */
		std::list<TuioObject*> getUntouchedObjects();

		/**
		 * Returns a List of all currently inactive TuioCursors
		 *
		 * @return  a List of all currently inactive TuioCursors
		 */
		std::list<TuioCursor*> getUntouchedCursors();

		/**
		 * Returns a List of all currently inactive TuioBlobs
		 *
		 * @return  a List of all currently inactive TuioBlobs
		 */
		std::list<TuioBlob*> getUntouchedBlobs();
		
		/**
		 * Calculates speed and acceleration values for all currently inactive TuioObjects
		 */
		void stopUntouchedMovingObjects();

		/**
		 * Calculates speed and acceleration values for all currently inactive TuioCursors
		 */
		void stopUntouchedMovingCursors();

		/**
		 * Calculates speed and acceleration values for all currently inactive TuioBlobs
		 */
		void stopUntouchedMovingBlobs();
		
		/**
		 * Removes all currently inactive TuioObjects from the TuioServer's internal list of TuioObjects
		 */
		void removeUntouchedStoppedObjects();

		/**
		 * Removes all currently inactive TuioCursors from the TuioServer's internal list of TuioCursors
		 */
		void removeUntouchedStoppedCursors();

		/**
		 * Removes all currently inactive TuioBlobs from the TuioServer's internal list of TuioBlobs
		 */
		void removeUntouchedStoppedBlobs();
		
		/**
		 * Returns the TuioObject closest to the provided coordinates
		 * or NULL if there isn't any active TuioObject
		 *
		 * @param	xp	the X coordinate of the reference point
		 * @param	yp	the Y coordinate of the reference point
		 * @return  the closest TuioObject to the provided coordinates or NULL
		 */
		TuioObject* getClosestTuioObject(float xp, float yp);
		
		/**
		 * Returns the TuioCursor closest to the provided coordinates
		 * or NULL if there isn't any active TuioCursor
		 *
		 * @param	xp	the X coordinate of the reference point
		 * @param	yp	the Y coordinate of the reference point
		 * @return  the closest TuioCursor corresponding to the provided coordinates or NULL
		 */
		TuioCursor* getClosestTuioCursor(float xp, float yp);

		/**
		 * Returns the TuioBlob closest to the provided coordinates
		 * or NULL if there isn't any active TuioBlob
		 *
		 * @param	xp	the X coordinate of the reference point
		 * @param	yp	the Y coordinate of the reference point
		 * @return  the closest TuioBlob corresponding to the provided coordinates or NULL
		 */
		TuioBlob* getClosestTuioBlob(float xp, float yp);
		
		/**
		 * The TuioServer prints verbose TUIO event messages to the console if set to true.
		 * @param	verbose	print verbose messages if set to true
		 */
		void setVerbose(bool verbose) { this->verbose=verbose; }
		
		/**
		 * Returns true if the TuioServer prints verbose TUIO event messages to the console.
		 * @return	true if verbose messages are enabled
		 */
		bool isVerbose() { return verbose; }

		/**
		 * Inverts the X and Y coordinates as well as the rotation angle of all outgoing TUIO messages.
		 *
		 * @param	ix	invert the X coordinate if set to true
		 * @param	iy	invert the Y coordinate if set to true
		 * @param	ia	invert the rotation angle if set to true
		 */
		void setInversion(bool ix, bool iy, bool ia) { 
			invert_x = ix; 
			invert_y = iy; 
			invert_a = ia; 
		};

		/**
		 * Inverts the X coordinate of all outgoing TUIO messages.
		 *
		 * @param	ix	invert the X coordinate if set to true
		 */
		void setInvertXpos(bool ix) { invert_x = ix; };
		
		/**
		 * Inverts the Y coordinate of all outgoing TUIO messages.
		 *
		 * @param	iy	invert the Y coordinate if set to true
		 */
		void setInvertYpos(bool iy) { invert_y = iy; };
		
		/**
		 * Inverts the rotation angle of all outgoing TUIO messages.
		 *
		 * @param	ia	invert the rotation angle if set to true
		 */
		void setInvertAngle(bool ia) { invert_a = ia; };
		
		/**
		 * Returns true if the X coordinate is currently inverted.
		 * @return	true if the X coordinate is currently inverted
		 */
		bool getInvertXpos() { return invert_x; };
		
		/**
		 * Returns true if the Y coordinate is currently inverted.
		 * @return	true if the Y coordinate is currently inverted
		 */
		bool getInvertYpos() { return invert_y; };
		
		/**
		 * Returns true if the rotation angle is currently inverted.
		 * @return	true if the rotation angle is currently inverted
		 */
		bool getInvertAngle() { return invert_a; };
		
		/**
		 * Removes all TuioObjects from the TuioServer's internal list of TuioObjects
		 */
		void resetTuioObjects();
		
		/**
		 * Removes all TuioCursors from the TuioServer's internal list of TuioCursors
		 */
		void resetTuioCursors();
		
		/**
		 * Removes all TuioBlobs from the TuioServer's internal list of TuioBlobs
		 */
		void resetTuioBlobs();		
		
	protected:
		std::list<TuioCursor*> freeCursorList;
		std::list<TuioCursor*> freeCursorBuffer;

		std::list<TuioBlob*> freeBlobList;
		std::list<TuioBlob*> freeBlobBuffer;

		TuioTime currentFrameTime;
		int currentFrame;
		int maxCursorID;
		int maxBlobID;
		int sessionID;

		bool updateObject;
		bool updateCursor;
		bool updateBlob;
		bool verbose;

		bool invert_x;
		bool invert_y;
		bool invert_a;
	};
}
#endif /* INCLUDED_TUIOMANAGER_H */
