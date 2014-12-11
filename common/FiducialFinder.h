/*  reacTIVision tangible interaction framework
    FiducialFinder.h
    Copyright (C) 2005-2014 Martin Kaltenbrunner <martin@tuio.org>

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

#ifndef FIDUCIALFINDER
#define FIDUCIALFINDER

#include <vector>
#include <list>
#include <sstream>
#include <string>

#include "FrameProcessor.h"
#include "TuioServer.h"
#include "MidiServer.h"
#include "FiducialObject.h"
#include "SDLinterface.h"
#include "floatpoint.h"

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#endif

class FiducialFinder: public FrameProcessor
{
public:
	FiducialFinder(MessageServer *server, const char* grid_cfg) {

		this->message_server = server;
		if (server->getType()==TUIO_SERVER) {
			tuio_server = (TuioServer*)server;
			midi_server = NULL;
		} else if (server->getType()==MIDI_SERVER) {
			midi_server = (MidiServer*)server;
			tuio_server = NULL;
		}

	if (strcmp(grid_cfg, "none" ) == 0 ) {
#ifdef __APPLE__
		char path[1024];
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
		CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
		CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);	
		CFRelease( mainBundleURL);
		CFRelease( cfStringRef);
		sprintf(full_path,"%s/Contents/Resources/calibration.grid",path);
		grid_config = full_path;
#else
		grid_config = "./calibration.grid";
#endif
	} else grid_config = grid_cfg;

		calibration = false;
		totalframes = 0;
		session_id = 0;

		currentSetting = INV_NONE;
		show_settings = false;
	}

	~FiducialFinder() {
		 if (initialized) delete dmap;
	}

	virtual void process(unsigned char *src, unsigned char *dest, SDL_Surface *display) = 0;

	bool init(int w, int h, int sb ,int db);
	void toggleFlag(int flag);
	void finish();

protected:
	std::list<FiducialObject> fiducialList;

	int session_id;
	long totalframes;

	const char* grid_config;
	int cell_width, cell_height;
	int grid_size_x, grid_size_y;
	#ifdef __APPLE__
	char full_path[1024];
	#endif

	bool calibration, show_grid, empty_grid;
	ShortPoint* dmap;

	void drawObject(int id, int xpos, int ypos, SDL_Surface *display,int state);
	void drawGrid(unsigned char *src, unsigned char *dest, SDL_Surface *display);
	void drawGUI(SDL_Surface *display);
	void computeGrid();

	TuioServer *tuio_server;
	MidiServer *midi_server;
	MessageServer *message_server;
	
	void sendTuioMessages();
	void sendMidiMessages();

	MessageListener::DisplayMode prevMode;
	bool show_settings;
	
private:
	enum InvertSetting { INV_NONE, INV_XPOS, INV_YPOS, INV_ANGLE };
	int currentSetting;

};

#endif
