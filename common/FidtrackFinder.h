/*  reacTIVision tangible interaction framework
    FidtrackFinder.h
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

#ifndef FIDTRACKFINDER
#define FIDTRACKFINDER

#include "FiducialFinder.h"
#include "FingerObject.h"
#include "segment.h"
#include "fidtrackX.h"
#define MAX_FIDUCIAL_COUNT 512

#define FINGER_ID -10

class FidtrackFinder: public FiducialFinder
{
public:
	FidtrackFinder(MessageServer *server, const char* tree_cfg, const char* grid_cfg, int finger_size, int finger_sens) : FiducialFinder (server,grid_cfg) {

#ifdef __APPLE__
		if (strcmp(tree_cfg,"none")!=0) {
			char app_path[1024];
 			CFBundleRef mainBundle = CFBundleGetMainBundle();
			CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
			CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
			CFStringGetCString( cfStringRef, app_path, 1024, kCFStringEncodingASCII);	
			CFRelease( mainBundleURL);
			CFRelease( cfStringRef);
			sprintf(mac_config,"%s/Contents/Resources/%s",app_path,tree_cfg);
			tree_config = mac_config;
		} else tree_config = tree_cfg;
#else
		tree_config = tree_cfg;
#endif

		average_finger_size = 0;
		detect_finger = true;

		if (finger_size>0) average_finger_size = finger_size;
		else detect_finger = false;
		finger_sensitivity = finger_sens/100.0f;
	};

	~FidtrackFinder() {
		if (initialized) {
			terminate_segmenter(&segmenter);
			terminate_treeidmap(&treeidmap);
			terminate_fidtrackerX(&fidtrackerx);
			//delete[] finger_buffer;
		}
	};

	void process(unsigned char *src, unsigned char *dest, SDL_Surface *display);
	bool init(int w ,int h, int sb, int db);
	void drawGUI(SDL_Surface *display);
	void toggleFlag(int flag);

	int getFingerSize() { return average_finger_size; };
	int getFingerSensitivity() { return (int)(finger_sensitivity*100); };

	std::list<FiducialObject> getCalibrationMarkers();
	std::list<FingerObject> getCalibrationPoints();
	void reset();

private:
	Segmenter segmenter;
	const char* tree_config;
		
	FiducialX fiducials[ MAX_FIDUCIAL_COUNT ];
	RegionX regions[ MAX_FIDUCIAL_COUNT*4 ];
	TreeIdMap treeidmap;
	FidtrackerX fidtrackerx;

  	std::list<FingerObject> fingerList;
	void sendCursorMessages();
	void printStatistics(long start_time);

	int check_finger(RegionX *finger, unsigned char* image, unsigned char* display);

	bool detect_finger;
	float average_leaf_size;
	float average_fiducial_size;
	int average_finger_size;
	float finger_sensitivity;

	bool setFingerSize, setFingerSensitivity;
	//unsigned char* finger_buffer;
#ifdef __APPLE__
    char mac_config[1024];
#endif
};

#endif
