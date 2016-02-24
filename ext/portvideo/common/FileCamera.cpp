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

CameraEngine* FileCamera::getCamera(CameraConfig *cam_cfg) {
	
	FILE* imagefile=fopen(cam_cfg->src, "rb");
	if (imagefile==NULL) return NULL;
	fclose(imagefile);
	return new FileCamera(cam_cfg);
}

bool FileCamera::initCamera() {
	
	int  max = 0;
	char header[32];
	char *param;
	
	FILE* imagefile=fopen(cfg->src, "r");
	if (imagefile==NULL) return false;
	
	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	if (strstr(header,"P5")!=NULL) cfg->cam_format = FORMAT_GRAY;
	else if (strstr(header,"P6")!=NULL) cfg->cam_format = FORMAT_RGB;
	else return NULL;
	
	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	param = strtok(header," "); if (param) cfg->cam_width = atoi(param);
	param = strtok(NULL," "); if (param) cfg->cam_height =  atoi(param);
	param = strtok(NULL," "); if (param) max = atoi(param);
	
	if (cfg->cam_height==0) 	{
		fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
		param = strtok(header," "); if (param) cfg->cam_height = atoi(param);
		param = strtok(NULL," "); if (param) max = atoi(param);
	}
	
	if (max==0) {
		fgets(header,32,imagefile);
		while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
		param = strtok(header," "); if (param) max = atoi(param);
	}
	
	if ((cfg->cam_width==0) || (cfg->cam_height==0) ) return false;
	cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
	
	size_t size;
	if (cfg->cam_format!=cfg->buf_format) {
		unsigned char *file_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->cam_format];
		size = fread(file_buffer, cfg->cam_format,cfg->cam_width*cfg->cam_height, imagefile);
		
		if (cfg->color) gray2rgb(cfg->cam_width, cfg->cam_height, file_buffer, cam_buffer);
		else rgb2gray(cfg->cam_width, cfg->cam_height, file_buffer, cam_buffer);
		delete []file_buffer;
	} else {
		size = fread(cam_buffer, cfg->cam_format,cfg->cam_width*cfg->cam_height, imagefile);
	}
	
	fclose(imagefile);
	if ((int)size!=cfg->cam_width*cfg->cam_height) return false;

	cfg->cam_fps = 1;
	setupFrame();
	return true;
}

unsigned char* FileCamera::getFrame()
{
	if (running) {
#ifdef WIN32
	Sleep(100);
#else
	usleep( 10000 ); // do 10fps
#endif
	} else running = true;
	return cam_buffer;
}

bool FileCamera::startCamera()
{
	//running = true;
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
#endif

