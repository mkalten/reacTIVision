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
#include "FileCamera.h"
#ifdef WIN32
#include <windows.h>
#endif 

FileCamera::FileCamera(const char* cfg): CameraEngine(cfg)
{
	cameraID = -1;
	
	cam_buffer = NULL;

	sprintf(cameraName,"FileCamera");

	running=false;
}

FileCamera::~FileCamera()
{
	if (cam_buffer!=NULL) delete cam_buffer;
}

bool FileCamera::findCamera() {
	readSettings();
	if (config.file==NULL) return false;
	FILE* imagefile=fopen(config.file, "rb");
	if (imagefile==NULL) return false;
	fclose(imagefile);
	return true;
}

bool FileCamera::initCamera() {	

	int gray = 0;
	char header[32];
	char *param;
	
	if (config.file==NULL) return false;
	FILE*  imagefile=fopen(config.file, "r");
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

	cam_buffer = new unsigned char[cam_width*cam_height*bytes];		
	size_t size = fread(cam_buffer, bytes,  cam_width*cam_height, imagefile);
	if ((int)size!=cam_width*cam_height*bytes) std::cerr << "wrong image lenght" << std::endl;
	fclose(imagefile);

	config.cam_width = cam_width;
	config.cam_height = cam_height;

	bytes = 1;
	colour = false;
	fps = 10;
	
	return true;
}

unsigned char* FileCamera::getFrame()
{
#ifdef WIN32
	Sleep(100);
#else 
	usleep( 100000 ); // simulate 10fps
#endif
	return cam_buffer;	
}

bool FileCamera::startCamera()
{
	running = true;
	return true;
}

bool FileCamera::stopCamera()
{
	running = false;
	return true;
}

bool FileCamera::stillRunning() {
	return running;
}

bool FileCamera::resetCamera()
{
  return (stopCamera() && startCamera());
}

bool FileCamera::closeCamera()
{
	return true;
}

int FileCamera::getCameraSettingStep(int mode) { 
	return 0;
}

bool FileCamera::setCameraSettingAuto(int mode, bool flag) {
	return false;
}

bool FileCamera::getCameraSettingAuto(int mode) {
    return false;
}

bool FileCamera::setDefaultCameraSetting(int mode) {
    return false;
}

int FileCamera::getDefaultCameraSetting(int mode) {
    return false;
}

bool FileCamera::setCameraSetting(int mode, int setting) {
	return false;
}

int FileCamera::getCameraSetting(int mode) {
	return 0;
}

int FileCamera::getMaxCameraSetting(int mode) {
	return 0;
}

int FileCamera::getMinCameraSetting(int mode) {
	return 0;
}

void FileCamera::showSettingsDialog() {
	return;
}
#endif

