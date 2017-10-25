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

#include "CameraTool.h"
#ifdef LINUX
#include <pwd.h>
#endif

void pv_sleep(int ms) {
	#ifndef WIN32
		usleep(ms*1000);
	#else
		Sleep(ms);
	#endif
}

CameraConfig CameraTool::cam_cfg = {};

void CameraTool::printConfig(std::vector<CameraConfig> cfg_list) {

	if (cfg_list.size()==0) {
		return;
	}

	int device = -1;
	int width = -1;
	int height = -1;
	float fps = -1;
	int format = -1;
	int frame_mode = -1;

	for (int i=0;i<(int)cfg_list.size();i++) {

		if (cfg_list[i].device != device) {
			if (device>=0) {
				printf("\b fps\n");
			}
			device = cfg_list[i].device;
			printf("  %d: %s\n",cfg_list[i].device,cfg_list[i].name);
			format = -1;
		}

		if ((cfg_list[i].cam_format != format) || (cfg_list[i].frame_mode != frame_mode)) {
			if (format>=0) {
				printf("\b fps\n");
			}
			format = cfg_list[i].cam_format;
			if (cfg_list[i].frame_mode<0) {
				printf("    format: %s", fstr[cfg_list[i].cam_format]);
			} else {
				printf("    format7_%d: %s", cfg_list[i].frame_mode,
					fstr[cfg_list[i].cam_format]);
			}
			// if (cfg_list[i].compress) {
			// 	printf(" (default)");
			// }
			width = height = fps = -1;
			printf("\n");
		}

		if ((cfg_list[i].cam_width != width) || (cfg_list[i].cam_height != height)) {
			if (width>0) {
				printf("\b fps\n");
			}
			printf("      %dx%d ",cfg_list[i].cam_width,cfg_list[i].cam_height);
			width = cfg_list[i].cam_width;
			height = cfg_list[i].cam_height;
			fps = INT_MAX;
		}

		if (cfg_list[i].frame_mode>=0) {
			printf("max|");
		} else if (cfg_list[i].cam_fps != fps) {
			if (int(cfg_list[i].cam_fps)==cfg_list[i].cam_fps) {
				printf("%d|",int(cfg_list[i].cam_fps));
			} else {
				printf("%.1f|",cfg_list[i].cam_fps);
			}
			fps = cfg_list[i].cam_fps;
		}
	}
	printf("\b fps\n");

}

std::vector<CameraConfig> CameraTool::findDevices() {

	std::vector<CameraConfig> dev_list;

	#ifdef __APPLE__
		std::vector<CameraConfig> sys_list = AVfoundationCamera::getCameraConfigs();
		dev_list.insert(dev_list.end(), sys_list.begin(), sys_list.end());
	#endif

	#ifdef LINUX
		std::vector<CameraConfig> sys_list = V4Linux2Camera::getCameraConfigs();
		dev_list.insert(dev_list.end(), sys_list.begin(), sys_list.end());
	#else // ps3eye for WIN32 and MACOS
		std::vector<CameraConfig> ps3eye_list = PS3EyeCamera::getCameraConfigs();
		dev_list.insert(dev_list.end(), ps3eye_list.begin(), ps3eye_list.end());
	#endif

	#ifdef WIN32
		std::vector<CameraConfig> sys_list = videoInputCamera::getCameraConfigs();
		dev_list.insert(dev_list.end(), sys_list.begin(), sys_list.end());
	#else // dc1394 for LINUX and MACOS
		std::vector<CameraConfig> dc1394_list = DC1394Camera::getCameraConfigs();
		dev_list.insert( dev_list.end(), dc1394_list.begin(), dc1394_list.end() );
	#endif

	// if(dev_list.size() > 1) {
	// 	std::vector<CameraConfig> multicam_list = MultiCamera::getCameraConfigs(dev_list);
	// 	dev_list.insert(
	// 		dev_list.end(),
	// 		multicam_list.begin(),
	// 		multicam_list.end()
	// 	);
	// }

	return dev_list;

}


void CameraTool::listDevices() {

	int dev_count;

	#ifdef __APPLE__
		dev_count = AVfoundationCamera::getDeviceCount();
		if (dev_count==0) {
			printf("no system camera found\n");
		} else {
			if (dev_count==1) {
				printf("1 system camera found:\n");
			} else  {
				printf("%d system cameras found:\n",dev_count);
			}
			printConfig(AVfoundationCamera::getCameraConfigs());
		}
	#endif

	#ifdef LINUX
		dev_count = V4Linux2Camera::getDeviceCount();
		if (dev_count==0) {
			printf("no system camera found\n");
		} else {
			if (dev_count==1) {
				printf("1 system camera found:\n");
			} else {
				printf("%d system cameras found:\n",dev_count);
			}
			printConfig(V4Linux2Camera::getCameraConfigs());
		}
	#else // ps3eye for WIN32 and MACOS
		dev_count = PS3EyeCamera::getDeviceCount();
		if (dev_count == 0) {
			printf("no PS3Eye camera found\n");
		} else {
			if (dev_count == 1) {
				printf("1 PS3Eye camera found:\n");
			} else  {
				printf("%d PS3Eye cameras found:\n", dev_count);
			}
			printConfig(PS3EyeCamera::getCameraConfigs());
		}
	#endif

	#ifdef WIN32
		dev_count = videoInputCamera::getDeviceCount();
		if (dev_count==0) {
			printf("no system camera found\n");
		} else {
			if (dev_count==1) {
				printf("1 system camera found:\n");
			} else {
				printf("%d system cameras found:\n",dev_count);
			}
			printConfig(videoInputCamera::getCameraConfigs());
		}
	#else // dc1394 for LINUX and MACOS
		dev_count = DC1394Camera::getDeviceCount();
		if (dev_count==0) {
			printf("no DC1394 camera found\n");
		} else {
			if (dev_count==1) {
				printf("1 DC1394 camera found:\n");
			} else  {
				printf("%d DC1394 cameras found:\n",dev_count);
			}
			printConfig(DC1394Camera::getCameraConfigs());
		}
	#endif

}

CameraEngine* CameraTool::getDefaultCamera() {

	initCameraConfig(&cam_cfg);

	#ifdef WIN32
		return videoInputCamera::getCamera(&cam_cfg);
	#endif

	#ifdef __APPLE__
		return AVfoundationCamera::getCamera(&cam_cfg);
	#endif

	#ifdef LINUX
		return V4Linux2Camera::getCamera(&cam_cfg);
	#endif

	return NULL;
}

CameraEngine* CameraTool::getCamera(CameraConfig *cam_cfg) {
	return CameraTool::getCamera(cam_cfg, true);
}

CameraEngine* CameraTool::getCamera(CameraConfig *cam_cfg, bool fallback) {

	CameraEngine* camera = NULL;
	int dev_count;

	if (cam_cfg->driver==DRIVER_MUTLICAM) {
		camera = MultiCamera::getCamera(cam_cfg);
		if (camera) {
			return camera;
		} else {
			#ifdef DEBUG
				std::cout << "MultiCamera::getCamera failed" << std::endl;
			#endif
		}
	}

	#ifndef NDEBUG
		if (cam_cfg->driver==DRIVER_FILE) {
			camera = FileCamera::getCamera(cam_cfg);
			if (camera) {
				return camera;
			}
		}

		if (cam_cfg->driver==DRIVER_FOLDER) {
			camera = FolderCamera::getCamera(cam_cfg);
			if (camera) {
				return camera;
			}
		}
	#endif

	#ifndef LINUX
		if (cam_cfg->driver == DRIVER_PS3EYE) {
			dev_count = PS3EyeCamera::getDeviceCount();

			if (dev_count == 0) {
				printf("no PS3Eye camera found\n");
			} else {
				camera = PS3EyeCamera::getCamera(cam_cfg);
			}

			if (camera) {
				return camera;
			}
		}
	#endif

	#ifdef WIN32
		dev_count = videoInputCamera::getDeviceCount();

		if (dev_count==0) {
			printf("no system camera found\n");
		} else {
			camera = videoInputCamera::getCamera(cam_cfg);
		}

		if (camera) {
			return camera;
		} else if (fallback) {
			std::cout << "fallback to default Camera" << std::endl;
			return getDefaultCamera();
		}
	#else
		if (cam_cfg->driver==DRIVER_DC1394) {
			dev_count = DC1394Camera::getDeviceCount();

			if (dev_count==0) {
				printf("no DC1394 camera found\n");
			} else {
				camera = DC1394Camera::getCamera(cam_cfg);
			}

			if (camera) {
				return camera;
			}
		}
	#endif

	#ifdef __APPLE__
		// default driver
		dev_count = AVfoundationCamera::getDeviceCount();

		if (dev_count==0) {
			printf("no system camera found\n");
			return NULL;
		} else {
			camera = AVfoundationCamera::getCamera(cam_cfg);
		}

		if (camera) {
			return camera;
		} else if (fallback) {
			std::cout << "fallback to default Camera" << std::endl;
			return getDefaultCamera();
		}
	#endif

	#ifdef LINUX
		// default driver
		dev_count = V4Linux2Camera::getDeviceCount();

		if (dev_count==0) {
			printf("no system camera found\n");
			return NULL;
		} else {
			camera = V4Linux2Camera::getCamera(cam_cfg);
		}

		if (camera) {
			return camera;
		} else if (fallback) {
			std::cout << "fallback to default Camera" << std::endl;
			return getDefaultCamera();
		}
	#endif

	return NULL;
}

void CameraTool::initCameraConfig(CameraConfig *cfg) {

	sprintf(cfg->src,"none");

	cfg->driver = DRIVER_DEFAULT;
	cfg->device = 0;
	cfg->cam_format = FORMAT_UNKNOWN;
	cfg->src_format = FORMAT_YUYV;
	cfg->buf_format = FORMAT_GRAY;

	cfg->cam_width = SETTING_MAX;
	cfg->cam_height = SETTING_MAX;
	cfg->cam_fps = SETTING_MAX;

	cfg->frame = false;
	cfg->frame_x = 0;
	cfg->frame_y = 0;
	cfg->frame_width = SETTING_MAX;
	cfg->frame_height = SETTING_MAX;
	cfg->frame_xoff = 0;
	cfg->frame_yoff = 0;
	cfg->frame_mode = -1;

	cfg->brightness = SETTING_DEFAULT;
	cfg->contrast = SETTING_DEFAULT;
	cfg->sharpness = SETTING_DEFAULT;
	cfg->gain = SETTING_DEFAULT;

	cfg->exposure = SETTING_DEFAULT;
	cfg->shutter = SETTING_DEFAULT;
	cfg->focus = SETTING_DEFAULT;
	cfg->white = SETTING_DEFAULT;
	cfg->gamma = SETTING_DEFAULT;
	cfg->powerline = SETTING_DEFAULT;
	cfg->backlight = SETTING_DEFAULT;

	cfg->hue = SETTING_DEFAULT;
	cfg->blue = SETTING_DEFAULT;
	cfg->red = SETTING_DEFAULT;
	cfg->green = SETTING_DEFAULT;

	cfg->force = false;
}

void CameraTool::setCameraConfig(CameraConfig *cfg) {

	if ((cam_cfg.driver != cfg->driver) || (cam_cfg.device != cfg->device)) {

		cam_cfg.brightness = SETTING_DEFAULT;
		cam_cfg.contrast = SETTING_DEFAULT;
		cam_cfg.sharpness = SETTING_DEFAULT;
		cam_cfg.gain = SETTING_DEFAULT;

		cam_cfg.exposure = SETTING_DEFAULT;
		cam_cfg.shutter = SETTING_DEFAULT;
		cam_cfg.focus = SETTING_DEFAULT;
		cam_cfg.white = SETTING_DEFAULT;
		cam_cfg.gamma = SETTING_DEFAULT;
		cam_cfg.powerline = SETTING_DEFAULT;
		cam_cfg.backlight = SETTING_DEFAULT;

		cam_cfg.hue = SETTING_DEFAULT;
		cam_cfg.blue = SETTING_DEFAULT;
		cam_cfg.red = SETTING_DEFAULT;
		cam_cfg.green = SETTING_DEFAULT;
	}

	strncpy(cam_cfg.name, cfg->name, 256);

	cam_cfg.driver = cfg->driver;
	cam_cfg.device = cfg->device;
	cam_cfg.cam_format = cfg->cam_format;

	cam_cfg.cam_width = cfg->cam_width;
	cam_cfg.cam_height = cfg->cam_height;
	cam_cfg.cam_fps = cfg->cam_fps;

	cam_cfg.force = cfg->force;
	// cam_cfg.flip_h = cfg->flip_h;
	// cam_cfg.flip_v = cfg->flip_v;

	cam_cfg.frame = cfg->frame;
	cam_cfg.frame_x = cfg->frame_x;
	cam_cfg.frame_y = cfg->frame_y;
	cam_cfg.frame_width = cfg->frame_width;
    cam_cfg.frame_height = cfg->frame_height;
    cam_cfg.frame_xoff = cfg->frame_xoff;
    cam_cfg.frame_yoff = cfg->frame_yoff;
	cam_cfg.frame_mode = cfg->frame_mode;

	// strncpy(cam_cfg.calib_grid_path, cfg->calib_grid_path, 1024);

    cam_cfg.childs = cfg->childs;
}


void CameraTool::whereIsConfig(const char* const cfgfilename, char* cfgfile) {

	#ifdef __APPLE__
	    char path[1024];
	    CFBundleRef mainBundle = CFBundleGetMainBundle();
	    CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
	    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
	    CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);
	    CFRelease( mainBundleURL);
	    CFRelease( cfStringRef);
	    sprintf(cfgfile,"%s/Contents/Resources/%s", path, cfgfilename);
	#elif !defined WIN32
	    sprintf(cfgfile, "./%s", cfgfilename);
	    if(access(cfgfile, F_OK) == 0) return;

	    sprintf(cfgfile, "/usr/share/portvideo/%s", cfgfilename);
	    if(access(cfgfile, F_OK) == 0) return;

	    sprintf(cfgfile, "/usr/local/share/portvideo/%s", cfgfilename);
	    if(access(cfgfile, F_OK) == 0) return;

	    sprintf(cfgfile, "/opt/share/portvideo/%s", cfgfilename);
	    if(access(cfgfile, F_OK) == 0) return;

		sprintf(cfgfile, "/opt/local/share/portvideo/%s", cfgfilename);
		if(access(cfgfile, F_OK) == 0) return;

	    *cfgfile = '\0';
	#else
	    sprintf(cfgfile, "./%s", cfgfilename);
	#endif
}

int CameraTool::mapCameraDriver(const char* const driver) {

    if(driver == NULL) return DRIVER_DEFAULT;

    if(strcmp(driver, "dc1394" ) == 0) {
		return DRIVER_DC1394;
    } else if(strcmp(driver, "ps3eye" ) == 0) {
		return DRIVER_PS3EYE;
    } else if(strcmp(driver, "file" ) == 0) {
		return DRIVER_FILE;
    } else if(strcmp(driver, "folder" ) == 0) {
		return DRIVER_FOLDER;
    } else if(strcmp(driver, "multicam") == 0) {
		return DRIVER_MUTLICAM;
    } else {
		return DRIVER_DEFAULT;
	}
}



CameraConfig* CameraTool::readSettings(const char* cfgfile) {

	initCameraConfig(&cam_cfg);

	if (strcmp(cfgfile, "default") == 0) {
        whereIsConfig("camera.xml", cam_cfg.path);
    } else {
		sprintf(cam_cfg.path, "%s", cfgfile);
	}

	#ifdef DEBUG
		std::cout << "camera configuration file: '" <<
		cam_cfg.path << "'" << std::endl;
	#endif

	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(cam_cfg.path);
	if (xml_settings.Error()) {
		std::cout << "Error loading camera configuration file: '" <<
		cam_cfg.path << "' (xml/file Error)" << std::endl;
		return &cam_cfg;
	}

	tinyxml2::XMLHandle docHandle( &xml_settings );
	tinyxml2::XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
	tinyxml2::XMLElement* camera_element = camera.ToElement();
	if (camera_element==NULL) {
		std::cout << "Error loading camera configuration from file: '" <<
		cam_cfg.path << "' (no camera element found)" << std::endl;
		return &cam_cfg;
	}

	readSettings(camera_element, cam_cfg);
    return &cam_cfg;
}

void CameraTool::readSettings(
	tinyxml2::XMLElement* camera_element, CameraConfig& cam_cfg
) {

	if (camera_element==NULL) {
		std::cout << "Error loading camera configuration from camera_element: '" <<
		camera_element << "' (no camera element)" << std::endl;
		return;
	}

	const char* driver = camera_element->Attribute("driver");
	cam_cfg.driver = mapCameraDriver(driver);

	if (camera_element->Attribute("id")!=NULL) {
		if (strcmp(camera_element->Attribute("id"), "auto" ) == 0) {
			cam_cfg.device=SETTING_AUTO;
		} else {
			cam_cfg.device = atoi(camera_element->Attribute("id"));
		}
	}

	if (camera_element->Attribute("src")!=NULL) {
		#ifdef __APPLE__
			sprintf(cam_cfg.src,"%s/../%s",path,camera_element->Attribute("src"));
		#else
			sprintf(cam_cfg.src,"%s",camera_element->Attribute("src"));
		#endif
	}

	tinyxml2::XMLElement* image_element = camera_element->FirstChildElement("capture")->ToElement();

	if (image_element!=NULL) {
		cam_cfg.color = readAttributeHandleBool(image_element, "color");

		if (image_element->Attribute("format")!=NULL) {
			for (int i=FORMAT_MAX;i>0;i--) {
				if (strcmp( image_element->Attribute("format"), fstr[i] ) == 0) {
					cam_cfg.cam_format = i;
				}
			}
		}

		cam_cfg.cam_width = readAttributeHandleInt(
			image_element,
			"width",
			SETTING_MAX,
			SETTING_MIN
		);
		cam_cfg.cam_height = readAttributeHandleInt(
			image_element,
			"height",
			SETTING_MAX,
			SETTING_MIN
		);
		cam_cfg.cam_fps = readAttributeHandleInt(
			image_element,
			"fps",
			SETTING_MAX,
			SETTING_MIN
		);

		cam_cfg.force = readAttributeHandleBool(image_element, "force");
	}

	tinyxml2::XMLElement* frame_element = camera_element->FirstChildElement("frame")->ToElement();
	if (frame_element != NULL) {
		cam_cfg.frame = true;

		if (frame_element->Attribute("x")!=NULL) {
			cam_cfg.frame_x = atoi(frame_element->Attribute("x"));
		}

		if (frame_element->Attribute("y")!=NULL) {
			cam_cfg.frame_y = atoi(frame_element->Attribute("y"));
		}


		cam_cfg.frame_height = readAttributeHandleInt(
			frame_element,
			"height",
			SETTING_MAX,
			0
		);
		cam_cfg.frame_width = readAttributeHandleInt(
			frame_element,
			"width",
			SETTING_MAX,
			0
		);

		cam_cfg.frame_xoff = readAttributeHandleInt(
			frame_element,
			"xoff",
			SETTING_MAX,
			0
		);
		cam_cfg.frame_yoff = readAttributeHandleInt(
			frame_element,
			"yoff",
			SETTING_MAX,
			0
		);

		cam_cfg.frame_mode = readAttributeHandleInt(
			frame_element,
			"mode",
			SETTING_MAX,
			0
		);
	}

	tinyxml2::XMLElement* settings_element = camera_element->FirstChildElement("settings")->ToElement();
	if (settings_element!=NULL) {

		cam_cfg.brightness = readAttribute(settings_element, "brightness");
		cam_cfg.contrast = readAttribute(settings_element, "contrast");
		cam_cfg.sharpness = readAttribute(settings_element, "sharpness");
		cam_cfg.gain = readAttribute(settings_element, "gain");

		cam_cfg.exposure = readAttribute(settings_element, "exposure");
		cam_cfg.shutter = readAttribute(settings_element, "shutter");
		cam_cfg.focus = readAttribute(settings_element, "focus");
		cam_cfg.white = readAttribute(settings_element, "white");
		cam_cfg.powerline = readAttribute(settings_element, "powerline");
		cam_cfg.backlight = readAttribute(settings_element, "backlight");
		cam_cfg.gamma = readAttribute(settings_element, "gamma");

		if (cam_cfg.color) {
			cam_cfg.saturation= readAttribute(settings_element, "saturation");
			cam_cfg.hue = readAttribute(settings_element, "hue");
			cam_cfg.red = readAttribute(settings_element, "red");
			cam_cfg.green = readAttribute(settings_element, "green");
			cam_cfg.blue = readAttribute(settings_element, "blue");
		} else {
			cam_cfg.saturation = SETTING_OFF;
			cam_cfg.hue = SETTING_OFF;
			cam_cfg.red = SETTING_OFF;
			cam_cfg.green = SETTING_OFF;
			cam_cfg.blue = SETTING_OFF;
		}
	}

	// tinyxml2::XMLElement* calibration_element = camera_element->FirstChildElement("calibration");
	// if (calibration_element != NULL) {
	// 	if (calibration_element->Attribute("file") != NULL) {
	// 		if (calibration_element->Attribute("file") != NULL) {
	// 			sprintf(cam_cfg.calib_grid_path, "%s", calibration_element->Attribute("file"));
	// 		}
	// 	}
	// }

	// check for childcam configurations
	// clear childs
    cam_cfg.childs.clear();
	// read childs
	tinyxml2::XMLElement* child_cam = camera_element->FirstChildElement("childcamera");
	while (child_cam != NULL) {
		CameraConfig child_cfg;
		initCameraConfig(&child_cfg);
		readSettings(child_cam, child_cfg);
		cam_cfg.childs.push_back(child_cfg);
		child_cam = child_cam->NextSiblingElement("childcamera");
	}

}

int CameraTool::readAttributeHandleInt(
	tinyxml2::XMLElement* settings_element, const char *attribute, int max, int min
) {
	int result = SETTING_DEFAULT;
	if (settings_element->Attribute(attribute)!=NULL) {
		if (strcmp( settings_element->Attribute(attribute), "max" ) == 0) {
			result = max;
		} else if (strcmp( settings_element->Attribute(attribute), "min" ) == 0) {
			result = min;
		} else {
			result = atoi(settings_element->Attribute(attribute));
		}
	}
	return result;
}

bool CameraTool::readAttributeHandleBool(
	tinyxml2::XMLElement* settings_element, const char *attribute
) {
	bool result = false;
	if (
		(settings_element->Attribute(attribute)!=NULL) &&
		( strcmp( settings_element->Attribute(attribute), "true" ) == 0 )
	) {
		result = true;
	}
	return result;
}

int CameraTool::readAttribute(tinyxml2::XMLElement* settings,const char *attribute) {

	if (settings->Attribute(attribute)!=NULL) {
		if (strcmp(settings->Attribute(attribute), "min" ) == 0) {
			return SETTING_MIN;
		} else if (strcmp(settings->Attribute(attribute), "max" ) == 0) {
			return SETTING_MAX;
		} else if (strcmp(settings->Attribute(attribute), "auto" ) == 0) {
			return SETTING_AUTO;
		} else if (strcmp(settings->Attribute(attribute), "default" ) == 0) {
			return SETTING_DEFAULT;
		} else {
			return atoi(settings->Attribute(attribute));
		}
	}

	return SETTING_DEFAULT;
}



void CameraTool::saveSettings() {

	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(cam_cfg.path);
	if (xml_settings.Error()) {
		std::cout << "Error saving camera configuration file: '" <<
		cam_cfg.path << "'" << std::endl;
		return;
	}

	tinyxml2::XMLHandle docHandle( &xml_settings );
	tinyxml2::XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
	tinyxml2::XMLElement* camera_element = camera.ToElement();

	saveSettings(cam_cfg, camera_element);

	xml_settings.SaveFile(cam_cfg.path);
	if ( xml_settings.Error() ) {
		std::cout << "Error saving camera configuration file: '"
		<< cam_cfg.path << "'" << std::endl;
	}
}

void CameraTool::saveSettings(CameraConfig& cam_cfg, tinyxml2::XMLElement* camera_element) {

	if (camera_element!=NULL) {
		camera_element->SetAttribute("driver", dstr[cam_cfg.driver]);
		camera_element->SetAttribute("id", cam_cfg.device);
	}

	tinyxml2::XMLElement* image_element = camera_element->FirstChildElement("capture")->ToElement();
	if (image_element!=NULL) {
		image_element->SetAttribute("format", fstr[cam_cfg.cam_format]);
		image_element->SetAttribute("width", cam_cfg.cam_width);
		image_element->SetAttribute("height", cam_cfg.cam_height);
		image_element->SetAttribute("fps", cam_cfg.cam_fps);
		saveAttributeHandleBool(image_element, "color", cam_cfg.color);
		// saveAttributeHandleBool(image_element, "flip_h", cam_cfg.flip_h);
		// saveAttributeHandleBool(image_element, "flip_v", cam_cfg.flip_v);
	}

	tinyxml2::XMLElement* settings_element = camera_element->FirstChildElement("settings")->ToElement();
	if (settings_element!=NULL) {

		saveAttributeHandleInt(settings_element, "brightness", cam_cfg.brightness);
		saveAttributeHandleInt(settings_element, "contrast", cam_cfg.contrast);
		saveAttributeHandleInt(settings_element, "sharpness", cam_cfg.sharpness);
		saveAttributeHandleInt(settings_element, "gain", cam_cfg.gain);

		saveAttributeHandleInt(settings_element, "exposure", cam_cfg.exposure);
		saveAttributeHandleInt(settings_element, "focus", cam_cfg.focus);
		saveAttributeHandleInt(settings_element, "shutter", cam_cfg.shutter);
		saveAttributeHandleInt(settings_element, "white", cam_cfg.white);
		saveAttributeHandleInt(settings_element, "backlight", cam_cfg.backlight);
		saveAttributeHandleInt(settings_element, "powerline", cam_cfg.powerline);
		saveAttributeHandleInt(settings_element, "gamma", cam_cfg.gamma);

		saveAttributeHandleInt(settings_element, "saturation", cam_cfg.saturation);
		saveAttributeHandleInt(settings_element, "hue", cam_cfg.hue);
		saveAttributeHandleInt(settings_element, "red", cam_cfg.red);
		saveAttributeHandleInt(settings_element, "green", cam_cfg.green);
		saveAttributeHandleInt(settings_element, "blue", cam_cfg.blue);

	}

	tinyxml2::XMLElement* frame_element = camera_element->FirstChildElement("frame")->ToElement();
	if (frame_element!=NULL) {
		// force save of frame attributes
		saveAttribute(frame_element, "x", cam_cfg.frame_x);
		saveAttribute(frame_element, "y", cam_cfg.frame_y);
		saveAttribute(frame_element, "width", cam_cfg.frame_width);
		saveAttribute(frame_element, "height", cam_cfg.frame_height);
		saveAttribute(frame_element, "xoff", cam_cfg.frame_xoff);
		saveAttribute(frame_element, "yoff", cam_cfg.frame_yoff);
		saveAttribute(frame_element, "mode", cam_cfg.frame_mode);
	}

	// tinyxml2::XMLElement* calibration_element = camera->FirstChildElement("calibration")->ToElement();
	// if (calibration_element!=NULL) {
	// 	saveAttribute(calibration_element, "file", cam_cfg.calib_grid_path);
	// }

	// this overwrittes all other things
	// std::vector<CameraConfig>::iterator iter;
	// tinyxml2::XMLDocument* doc = camera_element->GetDocument();
    // for(iter = cam_cfg.childs.begin(); iter != cam_cfg.childs.end(); iter++) {
    //     tinyxml2::XMLElement* child_cam_element = doc->NewElement("childcamera");
    //     saveSettings(*iter, child_cam_element);
    //     camera_element->LinkEndChild(child_cam_element);
    // }

	// check if we have childs.
	// if (cam_cfg.driver == DRIVER_MUTLICAM) {
	if (cam_cfg.childs.size() > 0) {
		// with this we try to not overwritte but chagne.
		tinyxml2::XMLElement* child_cam_element = camera_element->FirstChildElement("childcamera")->ToElement();
		std::vector<CameraConfig>::iterator iter;
		for(iter = cam_cfg.childs.begin(); iter != cam_cfg.childs.end(); iter++) {
			if (child_cam_element == NULL) {
				tinyxml2::XMLDocument* doc = camera_element->GetDocument();
				tinyxml2::XMLElement* child_cam_element = doc->NewElement("childcamera");
				saveSettings(*iter, child_cam_element);
		        camera_element->LinkEndChild(child_cam_element);
			} else {
				saveSettings(*iter, child_cam_element);
				child_cam_element = child_cam_element->NextSiblingElement("childcamera")->ToElement();
			}
		}
	}

}

void CameraTool::saveAttributeHandleInt(
	tinyxml2::XMLElement* settings_element, const char *attribute, int config_value
) {
	if (config_value != SETTING_OFF) {
		saveAttribute(settings_element, attribute, config_value);
	} else {
		settings_element->DeleteAttribute(attribute);
	}
}

void CameraTool::saveAttributeHandleBool(
	tinyxml2::XMLElement* settings_element, const char *attribute, int config_value
) {
	if (config_value) {
		settings_element->SetAttribute(attribute, "true");
	} else {
		settings_element->SetAttribute(attribute, "false");
	}
}

void CameraTool::saveAttribute(tinyxml2::XMLElement* settings,const char *attribute,int config) {

	if (config==SETTING_MIN) {
		settings->SetAttribute(attribute,"min");
	} else if (config==SETTING_MAX) {
		settings->SetAttribute(attribute,"max");
	} else if (config==SETTING_AUTO) {
		settings->SetAttribute(attribute,"auto");
	} else if (config==SETTING_DEFAULT) {
		settings->SetAttribute(attribute,"default");
	} else {
		char value[64];
		sprintf(value,"%d",config);
		settings->SetAttribute(attribute,value);
	}
}
