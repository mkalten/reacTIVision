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

#include "FolderCamera.h"
#ifdef WIN32
#include <windows.h>
#endif

FolderCamera::FolderCamera(CameraConfig *cam_cfg): CameraEngine(cam_cfg)
{
	cam_buffer = NULL;
	file_buffer = NULL;
	sprintf(cfg->name,"FolderCamera");
	running=false;
}

FolderCamera::~FolderCamera()
{
	if (cam_buffer!=NULL) delete cam_buffer;
	if (file_buffer!=NULL) delete file_buffer;
}

CameraEngine* FolderCamera::getCamera(CameraConfig *cam_cfg) {
	
	struct stat info;
	if (stat(cam_cfg->src,&info)!=0) return NULL;
	return new FolderCamera(cam_cfg);
}

bool FolderCamera::initCamera() {
	
	int  max = 0;
	char header[32];
	char file_name[256];
	char *param;
	
#ifdef WIN32
	WIN32_FIND_DATA results;
	char buf[255];
	sprintf(buf, "%s\\*", cfg->folder);
	HANDLE list = FindFirstFile(buf, &results);
	while (FindNextFile(list, &results)) {
		if ((strstr(results.cFileName,".pgm")!=NULL) || (strstr(results.cFileName,".ppm")!=NULL)) {
			sprintf(file_name,"%s\\%s",cfg->folder,results.cFileName);
			image_list.push_back(file_name);
		}
	}
	FindClose(list);
#else
	DIR *dp;
	struct dirent *ep;
	dp = opendir (cfg->src);
	if (dp != NULL) {
		while ((ep = readdir (dp))) {
			if ((strstr(ep->d_name,".pgm")!=NULL) || (strstr(ep->d_name,".ppm")!=NULL)) {
				sprintf(file_name,"%s/%s",cfg->src,ep->d_name);
				image_list.push_back(file_name);
			}
		}
		(void) closedir (dp);
	} else return false;
#endif
	
	if (image_list.size()==0) return false;
	image_list.sort();
	FILE* imagefile=fopen(image_list.begin()->c_str(),"rb");
	if (imagefile==NULL) return false;
	
	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	if (strstr(header,"P5")!=NULL) cfg->cam_format = FORMAT_GRAY;
	else if (strstr(header,"P6")!=NULL) cfg->cam_format = FORMAT_RGB;
	else return false;
	
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
	
	fclose(imagefile);
	if ((cfg->cam_width==0) || (cfg->cam_height==0) ) return false;
	//printf("%d %d\n",cfg->cam_width,cfg->cam_height);
	
	
	cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
	if (cfg->cam_format!=cfg->buf_format) file_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->cam_format];
	
	image_iterator = image_list.begin();
	
	if (cfg->cam_fps==SETTING_MIN) cfg->cam_fps = 15;
	if (cfg->cam_fps==SETTING_MAX) cfg->cam_fps = 30;
	setupFrame();
	return true;
}

unsigned char* FolderCamera::getFrame()
{
	int  max = 0;
	char header[32];
	char *param;
	
	int file_width =  0;
	int file_height = 0;

	FILE* imagefile=fopen(image_iterator->c_str(),"r");
	if (imagefile==NULL) return NULL;
	
	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	if ((cfg->cam_format==FORMAT_RGB) && (strstr(header,"P6")==NULL)) return NULL;
	else if  ((cfg->cam_format==FORMAT_GRAY) && (strstr(header,"P5")==NULL)) return NULL;
	
	fgets(header,32,imagefile);
	while (strstr(header,"#")!=NULL) fgets(header,32,imagefile);
	param = strtok(header," "); if (param) file_width = atoi(param);
	param = strtok(NULL," "); if (param) file_height =  atoi(param);
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

	if ((file_width!=cfg->cam_width) || (file_height!=cfg->cam_height) ) return NULL;
	
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
	if ((int)size!=cfg->cam_width*cfg->cam_height) return NULL;

	image_iterator++;
	if(image_iterator == image_list.end()) image_iterator=image_list.begin();
	
	
#ifdef WIN32
	Sleep((int)(1000/cfg->cam_fps));
#else
	usleep( (int)(1000000/cfg->cam_fps) );
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
#endif

