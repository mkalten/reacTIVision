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

#include "Main.h"
#include "FiducialFinder.h"
#include "FiducialObject.h"
#include "BlobObject.h"
#include "TuioCursor.h"
#include "segment.h"
#include "fidtrackX.h"
#include <assert.h>

#define MAX_FIDUCIAL_COUNT 1024

using namespace TUIO;

class FidtrackFinder: public FiducialFinder
{
public:
	FidtrackFinder(TUIO::TuioManager *manager, application_settings *config) : FiducialFinder (manager,config->grid_config) {
		
		#ifdef __APPLE__
		if (strstr(config->tree_config,".trees")!=NULL) {
			char app_path[1024];
			CFBundleRef mainBundle = CFBundleGetMainBundle();
			CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
			CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
			CFStringGetCString( cfStringRef, app_path, 1024, kCFStringEncodingASCII);
			CFRelease( mainBundleURL);
			CFRelease( cfStringRef);
			sprintf(tree_config,"%s/Contents/Resources/%s",app_path,config->tree_config);
		} else sprintf(tree_config,"%s",config->tree_config);
#else
		sprintf(tree_config,"%s",config->tree_config);
#endif
		
		position_threshold = 0.0f;
		rotation_threshold = 0.0f;

		average_finger_size = 0;
		detect_fingers = true;
		
		detect_blobs = true;
		send_finger_blobs = false;
		send_fiducial_blobs = false;
		
		if (config->finger_size>0) average_finger_size = config->finger_size;
		else detect_fingers = false;
		finger_sensitivity = config->finger_sensitivity/100.0f;
		
		
		max_blob_size = config->max_blob_size;
		min_blob_size = config->min_blob_size;
		if (max_blob_size==0) detect_blobs = false;
		
		send_fiducial_blobs = config->object_blobs;
		send_finger_blobs = config->cursor_blobs;
		
		detect_yamaarashi = config->yamaarashi;
		invert_yamaarashi = config->yama_flip;
		max_fiducial_id = config->max_fid;
		
		objFilter = config->obj_filter;
		curFilter = config->cur_filter;
		blbFilter = config->blb_filter;
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
	bool getYamaarashi() { return detect_yamaarashi; };
	
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
	int min_fiducial_size, max_fiducial_size;
	bool send_fiducial_blobs;

	bool detect_blobs;
	int max_blob_size;
	int min_blob_size;
	
	float position_threshold;
	float rotation_threshold;
	
	bool setFingerSize, setFingerSensitivity;
	bool setBlobSize, setObjectBlob, setFingerBlob;
	bool objFilter, curFilter, blbFilter;
	
	int max_fiducial_id;
	bool detect_yamaarashi;
	bool invert_yamaarashi;
	void decodeYamaarashi(FiducialX *yama, unsigned char *img, TuioTime ftime);
	float checkFinger(BlobObject *fblob);
};

#endif
