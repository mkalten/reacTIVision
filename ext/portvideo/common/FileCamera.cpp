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
#include "FileCamera.h"
#ifdef WIN32
#include <windows.h>
#endif 

FileCamera::FileCamera(CameraConfig *cam_cfg): CameraEngine(cam_cfg)
{
	cam_buffer = NULL;
	sprintf(cfg->name,"FileCamera");
	running=false;
}

FileCamera::~FileCamera()
{
	if (cam_buffer!=NULL) delete cam_buffer;
}

bool FileCamera::findCamera() {

	//if (config.file==NULL) return false;
	FILE* imagefile=fopen(cfg->file, "rb");
	if (imagefile==NULL) return false;
	fclose(imagefile);
	return true;
}

bool FileCamera::initCamera() {

	int gray = 0;
	char header[32];
	char *param;

	FILE* imagefile=fopen(cfg->file, "r");
	if (imagefile==NULL) return false;

 	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	if (strstr(header,"P5")==NULL) return false;

	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	param = strtok(header," "); if (param) cfg->cam_width = atoi(param);
	param = strtok(NULL," "); if (param) cfg->cam_height =  atoi(param);
	param = strtok(NULL," "); if (param) gray = atoi(param);

	if (cfg->cam_height==0) 	{
		fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
		param = strtok(header," "); if (param) cfg->cam_height = atoi(param);
		param = strtok(NULL," "); if (param) gray = atoi(param);
	}

	if (gray==0) {
		fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
		param = strtok(header," "); if (param) gray = atoi(param);
	}

    if ((cfg->cam_width==0) || (cfg->cam_height==0) ) return false;

    cfg->buf_format = FORMAT_GRAY;
    cfg->color = false;
    cfg->cam_fps = 1;

	cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
	size_t size = fread(cam_buffer, cfg->buf_format,cfg->cam_width*cfg->cam_height, imagefile);
	if ((int)size!=cfg->cam_width*cfg->cam_height*cfg->buf_format) std::cerr << "wrong image lenght" << std::endl;
	fclose(imagefile);

	setupFrame();
	return true;
}

unsigned char* FileCamera::getFrame()
{
#ifdef WIN32
	Sleep(1000);
#else
	usleep( 1000000 ); // do 1fps
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

bool FileCamera::showSettingsDialog(bool lock) {
	return lock;
}

bool FileCamera::hasCameraSetting(int mode) {
	return false;
}

bool FileCamera::hasCameraSettingAuto(int mode) {
	return false;
}

#endif

