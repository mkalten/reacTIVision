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

#ifndef FIDTRACKFINDER
#define FIDTRACKFINDER

#include "FiducialFinder.h"
#include "FiducialObject.h"
#include "BlobObject.h"
#include "TuioCursor.h"
#include "segment.h"
#include "fidtrackX.h"
#include <assert.h>

#define MAX_FIDUCIAL_COUNT 512

using namespace TUIO;

class FidtrackFinder: public FiducialFinder
{
public:
	FidtrackFinder(TUIO::TuioManager *manager, bool do_yama, const char* tree_cfg, const char* grid_cfg, int finger_size, int finger_sens, int blob_size, bool obj_blobs, bool cur_blobs) : FiducialFinder (manager,grid_cfg) {
		
		#ifdef __APPLE__
		if (strstr(tree_cfg,".trees")!=NULL) {
			char app_path[1024];
			CFBundleRef mainBundle = CFBundleGetMainBundle();
			CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
			CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
			CFStringGetCString( cfStringRef, app_path, 1024, kCFStringEncodingASCII);
			CFRelease( mainBundleURL);
			CFRelease( cfStringRef);
			sprintf(tree_config,"%s/Contents/Resources/%s",app_path,tree_cfg);
		} else sprintf(tree_config,"%s",tree_cfg);
#else
		sprintf(tree_config,"%s",tree_cfg);
#endif
		
		position_threshold = 0.0f;
		rotation_threshold = 0.0f;

		average_finger_size = 0;
		detect_fingers = true;
		
		detect_blobs = true;
		send_finger_blobs = false;
		send_fiducial_blobs = false;
		
		if (finger_size>0) average_finger_size = finger_size;
		else detect_fingers = false;
		finger_sensitivity = finger_sens/100.0f;
		
		if (blob_size>0) max_blob_size = blob_size;
		else detect_blobs = false;
		
		send_fiducial_blobs = obj_blobs;
		send_finger_blobs = cur_blobs;
		
		detect_yamaarashi = do_yama;
	};
	
	~FidtrackFinder() {
		if (initialized) {
			terminate_segmenter(&segmenter);
			terminate_treeidmap(&treeidmap);
			terminate_fidtrackerX(&fidtrackerx);
		}
	};
	
	void addUserInterface(UserInterface *uiface) {
		BlobObject::setInterface(uiface);
		ui = uiface;
	};
	
	void process(unsigned char *src, unsigned char *dest);
	bool init(int w ,int h, int sb, int db);
	void displayControl();
	bool toggleFlag(unsigned char flag, bool lock);
	
	int getFingerSize() { return average_finger_size; };
	int getFingerSensitivity() { return (int)(finger_sensitivity*100); };
	int getBlobSize() { return max_blob_size; };
	bool getFingerBlob() { return send_finger_blobs; };
	bool getFiducialBlob() { return send_fiducial_blobs; };
	
	void reset();
	
private:
	Segmenter segmenter;
	char tree_config[255];
	
	FiducialX fiducials[ MAX_FIDUCIAL_COUNT ];
	RegionX regions[ MAX_FIDUCIAL_COUNT*4 ];
	TreeIdMap treeidmap;
	FidtrackerX fidtrackerx;
	
	void printStatistics(TUIO::TuioTime frameTime);
	
	bool detect_fingers;
	int average_finger_size;
	float finger_sensitivity;
	bool send_finger_blobs;

	float average_fiducial_size;
	bool send_fiducial_blobs;

	bool detect_blobs;
	int max_blob_size;
	
	float position_threshold;
	float rotation_threshold;
	
	bool setFingerSize, setFingerSensitivity;
	bool setBlobSize, setObjectBlob, setFingerBlob;
	
	bool detect_yamaarashi;
	void decodeYamaarashi(FiducialX *yama, unsigned char *img);
	float checkFinger(BlobObject *fblob);
};

#endif
