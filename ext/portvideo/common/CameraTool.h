/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CAMERATOOL_H
#define CAMERATOOL_H

#include "MultiCamera.h"

#ifdef WIN32
#include <windows.h>
#include "videoInputCamera.h"
#include "PS3EyeCamera.h"
#endif

#ifdef LINUX
#include <stdio.h>
#include <stdlib.h>
#include "DC1394Camera.h"
#include "V4Linux2Camera.h"
#endif

#ifdef __APPLE__
#include <stdio.h>
#include <stdlib.h>
#include "AVfoundationCamera.h"
#include "DC1394Camera.h"
#include "PS3EyeCamera.h"
#endif

#ifndef NDEBUG
#include "FileCamera.h"
#include "FolderCamera.h"
#endif

#include <iostream>
#include <vector>
#include "tinyxml2.h"

void pv_sleep(int ms=1);

class CameraTool
{
public:

    static CameraEngine* getCamera(CameraConfig* cfg);
    static CameraEngine* getCamera(CameraConfig* cfg, bool fallback);
    static CameraEngine* getDefaultCamera();

    static std::vector<CameraConfig> findDevices();
	static void listDevices();
	static void printConfig(std::vector<CameraConfig>);

    static CameraConfig* readSettings(const char* cfgfile);
    static void readSettings(tinyxml2::XMLElement* camera_element, CameraConfig& cam_cfg);
    static void saveSettings();
    static void saveSettings(CameraConfig& cam_cfg, tinyxml2::XMLElement* camera_element);

    static void initCameraConfig(CameraConfig *cfg);
	static void setCameraConfig(CameraConfig *cfg);

    static void whereIsConfig(const char* const cfgfilename, char* cfgfile);
    static int mapCameraDriver(const char* const driver);

private:

    static CameraConfig cam_cfg;

    static int readAttribute(tinyxml2::XMLElement* settings,const char *attribute);

    static int readAttributeHandleInt(
    	tinyxml2::XMLElement* settings_element, const char *attribute, int max, int min
    );
    static bool readAttributeHandleBool(
    	tinyxml2::XMLElement* settings_element, const char *attribute
    );


    static void saveAttribute(tinyxml2::XMLElement* settings,const char *attribute,int config);

    static void saveAttributeHandleInt(
    	tinyxml2::XMLElement* settings_element, const char *attribute, int config_value
    );
    static void saveAttributeHandleBool(
    	tinyxml2::XMLElement* settings_element, const char *attribute, int config_value
    );

};

#endif
