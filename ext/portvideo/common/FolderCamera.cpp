/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>
 
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

#ifndef NDEBUG

#include "FolderCamera.h"
#ifdef WIN32
#include <windows.h>
#endif 

FolderCamera::FolderCamera(CameraConfig *cam_cfg): CameraEngine(cam_cfg)
{
	cam_buffer = NULL;
	sprintf(cfg->name,"FolderCamera");
	running=false;
}

FolderCamera::~FolderCamera()
{
	if (cam_buffer!=NULL) delete cam_buffer;
}

bool FolderCamera::findCamera() {

	//if (cfg->folder==NULL) return false;
	struct stat info;
	if (stat(cfg->folder,&info)!=0) return false;	
	return true;
}

bool FolderCamera::initCamera() {	

	int gray = 0;
	char header[32];
	char file_name[256];
	char *param;

#ifdef WIN32
	WIN32_FIND_DATA results;
	char buf[255];
    sprintf(buf, "%s\\*", cfg->folder);
	HANDLE list = FindFirstFile(buf, &results);
	while (FindNextFile(list, &results)) {
		if (strstr(results.cFileName,".pgm")!=NULL) {
			sprintf(file_name,"%s\\%s",cfg->folder,results.cFileName);
			image_list.push_back(file_name);
		}
		
	}
	FindClose(list);
#else
	DIR *dp;
	struct dirent *ep;
	dp = opendir (cfg->folder);
	if (dp != NULL) {
		while ((ep = readdir (dp))) {
			 if (strstr(ep->d_name,".pgm")!=NULL) {
				sprintf(file_name,"%s/%s",cfg->folder,ep->d_name);
				image_list.push_back(file_name);
			  }
		}
		(void) closedir (dp);
	} else return false;
#endif
    
    if (image_list.size()==0) return false;
 	FILE*  imagefile=fopen(image_list.begin()->c_str(),"rb");
	if (imagefile==NULL) return false;
		
	char *result = fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
	if (strstr(header,"P5")==NULL) return false;
		
	result = fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
	param = strtok(header," "); if (param) cfg->cam_width = atoi(param);
	param = strtok(NULL," "); if (param) cfg->cam_height =  atoi(param);
	param = strtok(NULL," "); if (param) gray = atoi(param);

	if (cfg->cam_height==0) 	{
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) cfg->cam_height = atoi(param);
		param = strtok(NULL," "); if (param) gray = atoi(param);
	}

	if (gray==0) {
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) gray = atoi(param);
	}

	if ((cfg->cam_width==0) || (cfg->cam_height==0) ) return false; 

	fclose(imagefile);

	cfg->buf_format = FORMAT_GRAY;
	cfg->color = false;
    
	if (cfg->cam_fps==SETTING_MIN) cfg->cam_fps = 15;
	if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps = 30;
    
	cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
	image_iterator = image_list.begin();
    
    setupFrame();
	return true;
}

unsigned char* FolderCamera::getFrame()
{
	int gray = 0;
	char header[32];
	char *param;

 	FILE*  imagefile=fopen(image_iterator->c_str(),"r");
	if (imagefile==NULL) return NULL;
		
	char *result = fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
	if (strstr(header,"P5")==NULL) return NULL;
		
	result = fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
	param = strtok(header," "); if (param) cfg->cam_width = atoi(param);
	param = strtok(NULL," "); if (param) cfg->cam_height =  atoi(param);
	param = strtok(NULL," "); if (param) gray = atoi(param);

	if (cfg->cam_height==0) 	{ 
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) cfg->cam_height = atoi(param);
		param = strtok(NULL," "); if (param) gray = atoi(param);
	}

	if (gray==0) {
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) gray = atoi(param);
	}

	if ((cfg->cam_width!=cfg->cam_width) || (cfg->cam_height!=cfg->cam_height) ) return NULL; 
	size_t size = fread(cam_buffer, cfg->buf_format,  cfg->cam_width*cfg->cam_height, imagefile);
	if ((int)size!=cfg->cam_width*cfg->cam_height*cfg->buf_format) std::cerr << "wrong image length" << std::endl;
	fclose(imagefile);

	image_iterator++;
	if(image_iterator == image_list.end()) image_iterator=image_list.begin();


#ifdef WIN32
	Sleep((int)(1000/cfg->cam_fps));
#else 
	usleep( (int)(100000/cfg->cam_fps) );
#endif

	return cam_buffer;	
}

bool FolderCamera::startCamera()
{
	running = true;
	return true;
}

bool FolderCamera::stopCamera()
{
	running = false;
	return true;
}

bool FolderCamera::stillRunning() {
	return running;
}

bool FolderCamera::resetCamera()
{
  return (stopCamera() && startCamera());
}

bool FolderCamera::closeCamera()
{
	return true;
}

int FolderCamera::getCameraSettingStep(int mode) { 
	return 0;
}

bool FolderCamera::setCameraSettingAuto(int mode, bool flag) {
	return false;
}

bool FolderCamera::getCameraSettingAuto(int mode) {
    return false;
}

bool FolderCamera::setDefaultCameraSetting(int mode) {
    return false;
}

int FolderCamera::getDefaultCameraSetting(int mode) {
    return false;
}

bool FolderCamera::setCameraSetting(int mode, int setting) {
	return false;
}

int FolderCamera::getCameraSetting(int mode) {
	return 0;
}

int FolderCamera::getMaxCameraSetting(int mode) {
	return 0;
}

int FolderCamera::getMinCameraSetting(int mode) {
	return 0;
}

bool FolderCamera::showSettingsDialog(bool lock) {
	return lock;
}

bool FolderCamera::hasCameraSetting(int mode) {
	return false;
}

bool FolderCamera::hasCameraSettingAuto(int mode) {
	return false;
}

#endif

