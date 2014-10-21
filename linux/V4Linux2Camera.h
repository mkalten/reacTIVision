/*  portVideo, a cross platform camera framework
    Copyright (C) 2005-2014 Martin Kaltenbrunner <martin@tuio.org>
    Copyright (C) 2007 Peter Eschler <peschler@gmail.com>

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

#ifndef V4Linux2Camera_H
#define V4Linux2Camera_H

#include "../common/CameraEngine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <turbojpeg.h>
#include <dirent.h>

class V4Linux2Camera : public CameraEngine
{
public:
	V4Linux2Camera(const char *cfg);
	~V4Linux2Camera();

	static void listDevices();

	bool findCamera();
	bool initCamera();
	bool startCamera();
	unsigned char* getFrame();
	bool stopCamera();
	bool stillRunning();
	bool resetCamera();
	bool closeCamera();

	int getCameraSettingStep(int mode);
	bool setCameraSettingAuto(int mode, bool flag);
	bool getCameraSettingAuto(int mode);
	int getDefaultCameraSetting(int mode);
	bool setDefaultCameraSetting(int mode);
	bool setCameraSetting(int mode, int value);
        int getCameraSetting(int mode);
        int getMaxCameraSetting(int mode);
        int getMinCameraSetting(int mode);

protected:
    bool requestBuffers();
    bool mapBuffers();
    bool unmapBuffers();

private:
    v4l2_buffer v4l2_buf;
    v4l2_capability v4l2_caps;
    v4l2_requestbuffers v4l2_reqbuffers;
    v4l2_format v4l2_form;
    v4l2_streamparm v4l2_parm;
    v4l2_control v4l2_ctrl;
    v4l2_queryctrl v4l2_query;

    struct Buffers {
      void *start;
      size_t length;
    };

    static const int nr_of_buffers = 3;
    Buffers buffers[nr_of_buffers];
    bool buffers_initialized;
    int pixelformat;

    int *comps;

	tjhandle _jpegDecompressor;
};

#endif
