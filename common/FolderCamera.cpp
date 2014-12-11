/*  portVideo, a cross platform camera framework
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
#ifndef NDEBUG

#include "FolderCamera.h"
#ifdef WIN32
#include <windows.h>
#endif 

FolderCamera::FolderCamera(const char* cfg): CameraEngine(cfg)
{
	cameraID = -1;
	
	cam_buffer = NULL;
	sprintf(cameraName,"FolderCamera");

	running=false;
}

FolderCamera::~FolderCamera()
{
	if (cam_buffer!=NULL) delete cam_buffer;
}

bool FolderCamera::findCamera() {
	readSettings();
	if (config.folder==NULL) return false;
	struct stat info;
	if (stat(config.folder,&info)!=0) return false;	
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
    sprintf(buf, "%s\\*", config.folder);
	HANDLE list = FindFirstFile(buf, &results);
	while (FindNextFile(list, &results)) {
		if (strstr(results.cFileName,".pgm")!=NULL) {
			sprintf(file_name,"%s\\%s",config.folder,results.cFileName);
			image_list.push_back(file_name);
		}
		
	}
	FindClose(list);
#else
	DIR *dp;
	struct dirent *ep;
	dp = opendir (config.folder);
	if (dp != NULL) {
		while ((ep = readdir (dp))) {
			 if (strstr(ep->d_name,".pgm")!=NULL) {
				sprintf(file_name,"%s/%s",config.folder,ep->d_name);
				image_list.push_back(file_name);
			  }
		}
		(void) closedir (dp);
	} else return false;
#endif

 	FILE*  imagefile=fopen(image_list.begin()->c_str(),"rb");
	if (imagefile==NULL) return false;
		
	char *result = fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
	if (strstr(header,"P5")==NULL) return false;
		
	result = fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
	param = strtok(header," "); if (param) cam_width = atoi(param);
	param = strtok(NULL," "); if (param) cam_height =  atoi(param);
	param = strtok(NULL," "); if (param) gray = atoi(param);

	if (cam_height==0) 	{ 
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) cam_height = atoi(param);
		param = strtok(NULL," "); if (param) gray = atoi(param);
	}

	if (gray==0) {
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) gray = atoi(param);
	}

	if ((cam_width==0) || (cam_height==0) ) return false; 

	fclose(imagefile);

	config.cam_width = cam_width;
	config.cam_height = cam_height;
	bytes = 1;
	colour = false;
	fps = 50;
	
	cam_buffer = new unsigned char[cam_width*cam_height*bytes];
	image_iterator = image_list.begin();
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
	param = strtok(header," "); if (param) cam_width = atoi(param);
	param = strtok(NULL," "); if (param) cam_height =  atoi(param);
	param = strtok(NULL," "); if (param) gray = atoi(param);

	if (cam_height==0) 	{ 
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) cam_height = atoi(param);
		param = strtok(NULL," "); if (param) gray = atoi(param);
	}

	if (gray==0) {
		result = fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) result = fgets(header,32,imagefile);
		param = strtok(header," "); if (param) gray = atoi(param);
	}

	if ((cam_width!=config.cam_width) || (cam_height!=config.cam_height) ) return NULL; 
	size_t size = fread(cam_buffer, bytes,  cam_width*cam_height, imagefile);
	if ((int)size!=cam_width*cam_height*bytes) std::cerr << "wrong image length" << std::endl;
	fclose(imagefile);

	image_iterator++;
	if(image_iterator == image_list.end()) image_iterator=image_list.begin();


// simulate ~30fps

#ifdef WIN32
	Sleep(33);
#else 
	usleep( 33000 ); 
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

void FolderCamera::showSettingsDialog() {
	return;
}
#endif

