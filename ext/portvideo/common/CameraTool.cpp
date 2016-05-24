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

CameraConfig CameraTool::cam_cfg = {};

void CameraTool::printConfig(std::vector<CameraConfig> cfg_list) {

	if(cfg_list.size()==0) return;
	
	int device = -1;
	int width = -1;
	int height = -1;
	float fps = -1;
	int format = -1;
	int frame_mode = -1;
	
	for (int i=0;i<(int)cfg_list.size();i++) {
		
		if (cfg_list[i].device != device) {
			if (device>=0) printf("\b fps\n");
			device = cfg_list[i].device;
			printf("  %d: %s\n",cfg_list[i].device,cfg_list[i].name);
			format = -1;
		}
		
		if ((cfg_list[i].cam_format != format) || (cfg_list[i].frame_mode != frame_mode)) {
			if (format>=0) printf("\b fps\n");
			format = cfg_list[i].cam_format;
			if(cfg_list[i].frame_mode<0) printf("    format: %s",fstr[cfg_list[i].cam_format]);
			else printf("    format7_%d: %s",cfg_list[i].frame_mode,fstr[cfg_list[i].cam_format]);
			//if(cfg_list[i].compress) printf(" (default)");
			width = height = fps = -1;
			printf("\n");
		}
		
		if ((cfg_list[i].cam_width != width) || (cfg_list[i].cam_height != height)) {
			if (width>0) printf("\b fps\n");
			printf("      %dx%d ",cfg_list[i].cam_width,cfg_list[i].cam_height);
			width = cfg_list[i].cam_width;
			height = cfg_list[i].cam_height;
			fps = INT_MAX;
		}
		
		if (cfg_list[i].frame_mode>=0) printf("max|");
		else if (cfg_list[i].cam_fps != fps) {
			if(int(cfg_list[i].cam_fps)==cfg_list[i].cam_fps)
				printf("%d|",int(cfg_list[i].cam_fps));
			else printf("%.1f|",cfg_list[i].cam_fps);
			fps = cfg_list[i].cam_fps;
		}
	} printf("\b fps\n");
	
}

std::vector<CameraConfig> CameraTool::findDevices() {

#ifdef WIN32
	std::vector<CameraConfig> dev_list = videoInputCamera::getCameraConfigs();
#endif
	
#ifdef __APPLE__
	
#ifdef __x86_64__
	std::vector<CameraConfig> dev_list = AVfoundationCamera::getCameraConfigs();
#else
	std::vector<CameraConfig> dev_list = QTKitCamera::getCameraConfigs();
#endif
	
	std::vector<CameraConfig> dc1394_list = DC1394Camera::getCameraConfigs();
	dev_list.insert( dev_list.end(), dc1394_list.begin(), dc1394_list.end() );
	
	std::vector<CameraConfig> ps3eye_list = PS3EyeCamera::getCameraConfigs();
	dev_list.insert( dev_list.end(), ps3eye_list.begin(), ps3eye_list.end() );

#endif
	
#ifdef LINUX
	
	std::vector<CameraConfig> dev_list = V4Linux2Camera::getCameraConfigs();
	std::vector<CameraConfig> dc1394_list = DC1394Camera::getCameraConfigs();
	dev_list.insert( dev_list.end(), dc1394_list.begin(), dc1394_list.end() );
	
#endif
	
	return dev_list;
	
}


void CameraTool::listDevices() {
	
	int dev_count;
#ifdef WIN32
	dev_count = videoInputCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(videoInputCamera::getCameraConfigs());
	}
#endif
	
#ifdef __APPLE__
	dev_count = DC1394Camera::getDeviceCount();
	if (dev_count==0) printf("no DC1394 camera found\n");
	else {
		if (dev_count==1) printf("1 DC1394 camera found:\n");
		else  printf("%d DC1394 cameras found:\n",dev_count);
		printConfig(DC1394Camera::getCameraConfigs());
	}
	
	dev_count = PS3EyeCamera::getDeviceCount();
	if (dev_count==0) printf("no PS3Eye camera found\n");
	else {
		if (dev_count==1) printf("1 PS3Eye camera found:\n");
		else  printf("%d PS3Eye cameras found:\n",dev_count);
		printConfig(PS3EyeCamera::getCameraConfigs());
	}
	
#ifdef __x86_64__
	dev_count = AVfoundationCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(AVfoundationCamera::getCameraConfigs());
	}
#else
	dev_count = QTKitCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(QTKitCamera::getCameraConfigs());
	}
#endif
#endif
	
#ifdef LINUX
	dev_count = DC1394Camera::getDeviceCount();
	if (dev_count==0) printf("no DC1394 camera found\n");
	else {
		if (dev_count==1) printf("1 DC1394 camera found:\n");
		else  printf("%d DC1394 cameras found:\n",dev_count);
		printConfig(DC1394Camera::getCameraConfigs());
	}

	dev_count = V4Linux2Camera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(V4Linux2Camera::getCameraConfigs());
	}
#endif

}

CameraEngine* CameraTool::getDefaultCamera() {
	
	initCameraConfig(&cam_cfg);
	
#ifdef WIN32
	return videoInputCamera::getCamera(&cam_cfg);
#endif
	
#ifdef __APPLE__
#ifdef __x86_64__
	return AVfoundationCamera::getCamera(&cam_cfg);
#else
	return QTKitCamera::getCamera(&cam_cfg);
#endif
#endif
	
#ifdef LINUX
	return V4Linux2Camera::getCamera(&cam_cfg);
#endif
	
	return NULL;
}

CameraEngine* CameraTool::getCamera(CameraConfig *cam_cfg) {
	
	CameraEngine* camera = NULL;
	int dev_count;
	
#ifndef NDEBUG
	
	if (cam_cfg->driver==DRIVER_FILE) {
		camera = FileCamera::getCamera(cam_cfg);
		if (camera) return camera;
	}

	if (cam_cfg->driver==DRIVER_FOLDER) {
		camera = FolderCamera::getCamera(cam_cfg);
		if (camera) return camera;
	}

#endif

#ifdef WIN32

	dev_count = videoInputCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else camera = videoInputCamera::getCamera(cam_cfg);
	if (camera) return camera;
	else return getDefaultCamera();

#endif

#ifdef __APPLE__

	if (cam_cfg->driver==DRIVER_DC1394) {
		dev_count = DC1394Camera::getDeviceCount();
		if (dev_count==0) printf("no DC1394 camera found\n");
		else camera = DC1394Camera::getCamera(cam_cfg);
		if (camera) return camera;
	}

	if (cam_cfg->driver==DRIVER_PS3EYE) {
		dev_count = PS3EyeCamera::getDeviceCount();
		if (dev_count==0) printf("no PS3Eye camera found\n");
		else camera = PS3EyeCamera::getCamera(cam_cfg);
		if (camera) return camera;
	}

#ifdef __x86_64__
	// default driver
	dev_count = AVfoundationCamera::getDeviceCount();
	if (dev_count==0) {
		printf("no system camera found\n");
		return NULL;
	} else camera = AVfoundationCamera::getCamera(cam_cfg);
	if (camera) return camera;
	else return getDefaultCamera();
#else
	// default driver
	dev_count = QTKitCamera::getDeviceCount();
	if (dev_count==0) {
		printf("no system camera found\n");
		return NULL;
	} else camera = QTKitCamera::getCamera(cam_cfg);
	if (camera) return camera;
	else return getDefaultCamera();
#endif
#endif

#ifdef LINUX

	if (cam_cfg->driver==DRIVER_DC1394) {
		dev_count = DC1394Camera::getDeviceCount();
		if (dev_count==0) printf("no DC1394 camera found\n");
		else camera = DC1394Camera::getCamera(cam_cfg);
		if (camera) return camera;
	}

	// default driver
	dev_count = V4Linux2Camera::getDeviceCount();
	if (dev_count==0) {
		printf("no system camera found\n");
		return NULL;
	} else camera = V4Linux2Camera::getCamera(cam_cfg);
	if (camera) return camera;
	else return getDefaultCamera();

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

	cam_cfg.driver = cfg->driver;
	cam_cfg.device = cfg->device;
	cam_cfg.cam_format = cfg->cam_format;
	
	cam_cfg.cam_width = cfg->cam_width;
	cam_cfg.cam_height = cfg->cam_height;
	cam_cfg.cam_fps = cfg->cam_fps;
	
	cam_cfg.frame = cfg->frame;
	cam_cfg.frame_width = SETTING_MAX;
	cam_cfg.frame_height = SETTING_MAX;
	cam_cfg.frame_xoff = 0;
	cam_cfg.frame_yoff = 0;
	cam_cfg.frame_mode = cfg->frame_mode;
	
	cam_cfg.force = cfg->force;
}

CameraConfig* CameraTool::readSettings(const char* cfgfile) {

	char path[1024];
	initCameraConfig(&cam_cfg);
	if (strcmp( cfgfile, "default" ) == 0) {
#ifdef __APPLE__
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
		CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
		CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);
		CFRelease( mainBundleURL);
		CFRelease( cfStringRef);
		sprintf(cam_cfg.path,"%s/Contents/Resources/camera.xml",path);
#elif defined LINUX
		struct passwd *pw = getpwuid(getuid());
		sprintf(path,"%s/.portvideo/camera.xml",pw->pw_dir);

		sprintf(cam_cfg.path,"./camera.xml");
		if (access (cam_cfg.path, F_OK )!=0) sprintf(cam_cfg.path,"%s",path);
		if (access (cam_cfg.path, F_OK )!=0) sprintf(cam_cfg.path,"/usr/share/portvideo/camera.xml");
		if (access (cam_cfg.path, F_OK )!=0) sprintf(cam_cfg.path,"/usr/local/share/portvideo/camera.xml");
		if (access (cam_cfg.path, F_OK )!=0) sprintf(cam_cfg.path,"/opt/share/portvideo/camera.xml");
		if (access (cam_cfg.path, F_OK )!=0) sprintf(cam_cfg.path,"/opt/local/share/portvideo/camera.xml");
#else
		sprintf(cam_cfg.path,"./camera.xml");
#endif
	} else sprintf(cam_cfg.path,"%s",cfgfile);
	
	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(cam_cfg.path);
	if( xml_settings.Error() )
	{
		std::cout << "Error loading camera configuration file: " << cam_cfg.path << std::endl;
		return &cam_cfg;
	}
	
	tinyxml2::XMLHandle docHandle( &xml_settings );
	tinyxml2::XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
	tinyxml2::XMLElement* camera_element = camera.ToElement();
	
	if( camera_element==NULL )
	{
		std::cout << "Error loading camera configuration file: " << cam_cfg.path << std::endl;
		return &cam_cfg;
	}
	
	if(camera_element->Attribute("driver")!=NULL) {
		if (strcmp(camera_element->Attribute("driver"), "dc1394" ) == 0) cam_cfg.driver=DRIVER_DC1394;
		else if (strcmp(camera_element->Attribute("driver"), "ps3eye" ) == 0) cam_cfg.driver=DRIVER_PS3EYE;
		else if (strcmp(camera_element->Attribute("driver"), "file" ) == 0) cam_cfg.driver=DRIVER_FILE;
		else if (strcmp(camera_element->Attribute("driver"), "folder" ) == 0) cam_cfg.driver=DRIVER_FOLDER;
	}
	
	if(camera_element->Attribute("id")!=NULL) {
		if (strcmp(camera_element->Attribute("id"), "auto" ) == 0) cam_cfg.device=SETTING_AUTO;
		else cam_cfg.device = atoi(camera_element->Attribute("id"));
	}
	
	if(camera_element->Attribute("src")!=NULL) {
#ifdef __APPLE__
		sprintf(cam_cfg.src,"%s/../%s",path,camera_element->Attribute("src"));
#else
		sprintf(cam_cfg.src,"%s",camera_element->Attribute("src"));
#endif
	}
	
	tinyxml2::XMLElement* image_element = camera.FirstChildElement("capture").ToElement();
	
	if (image_element!=NULL) {
		if ((image_element->Attribute("color")!=NULL) && ( strcmp( image_element->Attribute("color"), "true" ) == 0 )) cam_cfg.color = true;
		
		if (image_element->Attribute("format")!=NULL) {
			for (int i=FORMAT_MAX;i>0;i--) {
				if (strcmp( image_element->Attribute("format"), fstr[i] ) == 0) cam_cfg.cam_format = i;
			}
		}
		
		if(image_element->Attribute("width")!=NULL) {
			if (strcmp( image_element->Attribute("width"), "max" ) == 0) cam_cfg.cam_width = SETTING_MAX;
			else if (strcmp( image_element->Attribute("width"), "min" ) == 0) cam_cfg.cam_width = SETTING_MIN;
			else cam_cfg.cam_width = atoi(image_element->Attribute("width"));
		}
		if(image_element->Attribute("height")!=NULL) {
			if (strcmp( image_element->Attribute("height"), "max" ) == 0) cam_cfg.cam_height = SETTING_MAX;
			else if (strcmp( image_element->Attribute("height"), "min" ) == 0) cam_cfg.cam_height = SETTING_MIN;
			else cam_cfg.cam_height = atoi(image_element->Attribute("height"));
		}
		if(image_element->Attribute("fps")!=NULL) {
			if (strcmp( image_element->Attribute("fps"), "max" ) == 0) cam_cfg.cam_fps = SETTING_MAX;
			else if (strcmp( image_element->Attribute("fps"), "min" ) == 0) cam_cfg.cam_fps = SETTING_MIN;
			else cam_cfg.cam_fps = atof(image_element->Attribute("fps"));
		}
		if ((image_element->Attribute("force")!=NULL) && ( strcmp( image_element->Attribute("force"), "true" ) == 0 )) cam_cfg.force = true;
	}
	
	tinyxml2::XMLElement* frame_element = camera.FirstChildElement("frame").ToElement();
	if (frame_element!=NULL) {
		cam_cfg.frame = true;
		
		if(frame_element->Attribute("width")!=NULL) {
			if (strcmp( frame_element->Attribute("width"), "max" ) == 0) cam_cfg.frame_width = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("width"), "min" ) == 0) cam_cfg.frame_width = 0;
			else cam_cfg.frame_width = atoi(frame_element->Attribute("width"));
		}
		
		if(frame_element->Attribute("height")!=NULL) {
			if (strcmp( frame_element->Attribute("height"), "max" ) == 0) cam_cfg.frame_height = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("height"), "min" ) == 0) cam_cfg.frame_height = 0;
			else cam_cfg.frame_height = atoi(frame_element->Attribute("height"));
		}
		
		if(frame_element->Attribute("xoff")!=NULL) {
			if (strcmp( frame_element->Attribute("xoff"), "max" ) == 0) cam_cfg.frame_xoff = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("xoff"), "min" ) == 0) cam_cfg.frame_xoff = 0;
			else cam_cfg.frame_xoff = atoi(frame_element->Attribute("xoff"));
		}
		
		if(frame_element->Attribute("yoff")!=NULL) {
			if (strcmp( frame_element->Attribute("yoff"), "max" ) == 0) cam_cfg.frame_yoff = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("yoff"), "min" ) == 0) cam_cfg.frame_yoff = 0;
			else cam_cfg.frame_yoff = atoi(frame_element->Attribute("yoff"));
		}
		
		if(frame_element->Attribute("mode")!=NULL) {
			if (strcmp( frame_element->Attribute("mode"), "max" ) == 0) cam_cfg.frame_mode = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("mode"), "min" ) == 0) cam_cfg.frame_mode = 0;
			else cam_cfg.frame_mode = atoi(frame_element->Attribute("mode"));
		}
	}
	
	tinyxml2::XMLElement* settings_element = camera.FirstChildElement("settings").ToElement();
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
	
	return &cam_cfg;
}

int CameraTool::readAttribute(tinyxml2::XMLElement* settings,const char *attribute) {
	
	if(settings->Attribute(attribute)!=NULL) {
		if (strcmp(settings->Attribute(attribute), "min" ) == 0) return SETTING_MIN;
		else if (strcmp(settings->Attribute(attribute), "max" ) == 0) return SETTING_MAX;
		else if (strcmp(settings->Attribute(attribute), "auto" ) == 0) return SETTING_AUTO;
		else if (strcmp(settings->Attribute(attribute), "default" ) == 0) return SETTING_DEFAULT;
		else return atoi(settings->Attribute(attribute));
	}
	
	return SETTING_DEFAULT;
}

void CameraTool::saveSettings() {
	
	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(cam_cfg.path);
	if( xml_settings.Error() )
	{
		std::cout << "Error saving camera configuration file: " << cam_cfg.path << std::endl;
		return;
	}
	
	tinyxml2::XMLHandle docHandle( &xml_settings );
	tinyxml2::XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
	tinyxml2::XMLElement* camera_element = camera.ToElement();
	
	
	if( camera_element!=NULL )
	{
		camera_element->SetAttribute("driver",dstr[cam_cfg.driver]);
		camera_element->SetAttribute("id",cam_cfg.device);
	}
	
	tinyxml2::XMLElement* image_element = camera.FirstChildElement("capture").ToElement();
	if (image_element!=NULL) {
		image_element->SetAttribute("format",fstr[cam_cfg.cam_format]);
		image_element->SetAttribute("width",cam_cfg.cam_width);
		image_element->SetAttribute("height",cam_cfg.cam_height);
		image_element->SetAttribute("fps",cam_cfg.cam_fps);
	}
	
	tinyxml2::XMLElement* settings_element = camera.FirstChildElement("settings").ToElement();
	if (settings_element!=NULL) {
		
		if (cam_cfg.brightness!=SETTING_OFF) saveAttribute(settings_element, "brightness", cam_cfg.brightness);
		else settings_element->DeleteAttribute("brightness");
		if (cam_cfg.contrast!=SETTING_OFF) saveAttribute(settings_element, "contrast", cam_cfg.contrast);
		else settings_element->DeleteAttribute("contrast");
		if (cam_cfg.sharpness!=SETTING_OFF) saveAttribute(settings_element, "sharpness", cam_cfg.sharpness);
		else settings_element->DeleteAttribute("sharpness");
		if (cam_cfg.gain!=SETTING_OFF) saveAttribute(settings_element, "gain", cam_cfg.gain);
		else settings_element->DeleteAttribute("gain");
		
		if (cam_cfg.exposure!=SETTING_OFF) saveAttribute(settings_element, "exposure", cam_cfg.exposure);
		else settings_element->DeleteAttribute("exposure");
		if (cam_cfg.focus!=SETTING_OFF) saveAttribute(settings_element, "focus", cam_cfg.focus);
		else settings_element->DeleteAttribute("focus");
		if (cam_cfg.shutter!=SETTING_OFF) saveAttribute(settings_element, "shutter", cam_cfg.shutter);
		else settings_element->DeleteAttribute("shutter");
		if (cam_cfg.white!=SETTING_OFF) saveAttribute(settings_element, "white", cam_cfg.white);
		else settings_element->DeleteAttribute("white");
		if (cam_cfg.backlight!=SETTING_OFF) saveAttribute(settings_element, "backlight", cam_cfg.backlight);
		else settings_element->DeleteAttribute("backlight");
		if (cam_cfg.powerline!=SETTING_OFF) saveAttribute(settings_element, "powerline", cam_cfg.powerline);
		else settings_element->DeleteAttribute("powerline");
		if (cam_cfg.gamma!=SETTING_OFF) saveAttribute(settings_element, "gamma", cam_cfg.gamma);
		else settings_element->DeleteAttribute("gamma");
		
		if (cam_cfg.saturation!=SETTING_OFF) saveAttribute(settings_element, "saturation", cam_cfg.saturation);
		else settings_element->DeleteAttribute("saturation");
		if (cam_cfg.hue!=SETTING_OFF) saveAttribute(settings_element, "hue", cam_cfg.hue);
		else settings_element->DeleteAttribute("hue");
		if (cam_cfg.red!=SETTING_OFF) saveAttribute(settings_element, "red", cam_cfg.red);
		else settings_element->DeleteAttribute("red");
		if (cam_cfg.green!=SETTING_OFF) saveAttribute(settings_element, "green", cam_cfg.green);
		else settings_element->DeleteAttribute("green");
		if (cam_cfg.blue!=SETTING_OFF) saveAttribute(settings_element, "blue", cam_cfg.green);
		else settings_element->DeleteAttribute("blue");
		
	}
	
	tinyxml2::XMLElement* frame_element = camera.FirstChildElement("frame").ToElement();
	if (frame_element!=NULL) {
		
		if (cam_cfg.frame_mode>=0) saveAttribute(frame_element, "mode", cam_cfg.frame_mode);
		else frame_element->DeleteAttribute("mode");
	}
	
	xml_settings.SaveFile(cam_cfg.path);
	if( xml_settings.Error() ) std::cout << "Error saving camera configuration file: "  << cam_cfg.path << std::endl;
	
}

void CameraTool::saveAttribute(tinyxml2::XMLElement* settings,const char *attribute,int config) {

	if (config==SETTING_MIN) settings->SetAttribute(attribute,"min");
	else if (config==SETTING_MAX) settings->SetAttribute(attribute,"max");
	else if (config==SETTING_AUTO) settings->SetAttribute(attribute,"auto");
	else if (config==SETTING_DEFAULT) settings->SetAttribute(attribute,"default");
	else {
		char value[64];
		sprintf(value,"%d",config);
		settings->SetAttribute(attribute,value);
	}
}

