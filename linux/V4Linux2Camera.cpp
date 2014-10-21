/* portVideo, a cross platform camera framework
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

#include "V4Linux2Camera.h"

V4Linux2Camera::V4Linux2Camera(const char* cfg) : CameraEngine(cfg)
{
    cameraID = -1;

    comps = new int;

    cam_buffer = NULL;
    crop_buffer = NULL;
    running = false;
    buffers_initialized = false;
}

V4Linux2Camera::~V4Linux2Camera(void)
{
    if (v4l2_form.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) tjDestroy(_jpegDecompressor);
    if (cam_buffer) delete cam_buffer;
    if (crop_buffer) delete crop_buffer;
}

bool V4Linux2Camera::initCamera() {

    if (cameraID < 0) return false;
    memset(&v4l2_form, 0, sizeof(v4l2_format));

    // select pixelformat
    for (int i=0;;i++) {
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(cameraID,VIDIOC_ENUM_FMT,&fmtdesc)) break;

        if ((config.compress) && (fmtdesc.pixelformat==V4L2_PIX_FMT_MJPEG)) {
            v4l2_form.fmt.pix.pixelformat = fmtdesc.pixelformat;
            break;
        } else if (!(config.compress) && ((fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_UYVY) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV420) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV410))) {
            v4l2_form.fmt.pix.pixelformat = fmtdesc.pixelformat;
            break;
        }
     }

    cam_width = 0;
    cam_height = 0;
    int max_width = 0;
    int max_height = 0;
    int min_width = INT_MAX;
    int min_height = INT_MAX;

    // select size
    for (int i=0;;i++) {
        struct v4l2_frmsizeenum frmsize;
        memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
        frmsize.index = i;
        frmsize.pixel_format  = v4l2_form.fmt.pix.pixelformat;
        if (-1 == ioctl(cameraID,VIDIOC_ENUM_FRAMESIZES,&frmsize)) break;

        if (frmsize.discrete.width>max_width) max_width=frmsize.discrete.width;
        if (frmsize.discrete.height>max_height) max_height=frmsize.discrete.height;
        if (frmsize.discrete.width<min_width) min_width=frmsize.discrete.width;
        if (frmsize.discrete.height<min_height) min_height=frmsize.discrete.height;

        if ((config.cam_width==frmsize.discrete.width) && (config.cam_height==frmsize.discrete.height)) {
            cam_width = frmsize.discrete.width;
            cam_height = frmsize.discrete.height;
            break;
        }
    }

    if (config.cam_width==SETTING_MAX) cam_width = max_width;
    if (config.cam_height==SETTING_MAX) cam_height = max_height;
    if (config.cam_width==SETTING_MIN) cam_width = min_width;
    if (config.cam_height==SETTING_MIN) cam_height = min_height;
    if (cam_width==0) cam_width = min_width;
    if (cam_height==0) cam_height = min_height;

    fps = 0;
    float max_fps = 0;
    float min_fps = INT_MAX;

    // select fps
    float last_fps = 0.0f;
    for (int i=0;;i++) {
        struct v4l2_frmivalenum frmival;
        memset(&frmival, 0, sizeof(v4l2_frmivalenum));
        frmival.index = i;
        frmival.pixel_format = v4l2_form.fmt.pix.pixelformat;
        frmival.width = cam_width;
        frmival.height = cam_height;
        if (-1 == ioctl(cameraID,VIDIOC_ENUM_FRAMEINTERVALS,&frmival)) break;

        float frm_fps = frmival.discrete.denominator/(float)frmival.discrete.numerator;
	if(frm_fps==last_fps) break;
        last_fps=frm_fps;

        if(frm_fps>max_fps) max_fps = frm_fps;
        if(frm_fps<min_fps) min_fps = frm_fps;
    }

    if (config.cam_fps==SETTING_MAX) fps = max_fps;
    if (config.cam_fps==SETTING_MIN) fps = min_fps;
    if (config.cam_fps>max_fps) fps = max_fps;
    if (config.cam_fps<min_fps) fps = min_fps;
    if (fps==0) fps = config.cam_fps;

    // try to set the desired dimensions
    v4l2_form.fmt.pix.width  = cam_width;
    v4l2_form.fmt.pix.height = cam_height;
    v4l2_parm.parm.capture.timeperframe.numerator = 1;
    v4l2_parm.parm.capture.timeperframe.denominator = int(fps);

    v4l2_form.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (cameraID, VIDIOC_S_FMT, &v4l2_form)) {
        printf("error setting pixel format: %s\n" , strerror(errno));
        return false;
    }

    // use the settings we got from the driver
    pixelformat = v4l2_form.fmt.pix.pixelformat;
    cam_width = v4l2_form.fmt.pix.width;
    cam_height = v4l2_form.fmt.pix.height;
    fps = v4l2_parm.parm.capture.timeperframe.denominator;

    if (v4l2_form.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) _jpegDecompressor = tjInitDecompress();
    applyCameraSettings();

    if (!requestBuffers()) {
        printf("Error requesting buffers.\n");
        return false;
    }

    if (!mapBuffers()) {
        printf("Unable to mmap buffers.\n");
        return false;
    }

    setupFrame();
    if (config.frame) crop_buffer = new unsigned char[frame_width*frame_height];
    cam_buffer = new unsigned char[cam_width*cam_height];
    buffers_initialized = true;
    return true;
}

int v4lfilter(const struct dirent *dir)
{
	const char *name = dir->d_name;
	int len = strlen(name);
	if(len >= 0) {
		if (strncmp(name, "video", 5) == 0) return 1;
	}

	return 0;
}

void V4Linux2Camera::listDevices() {

    char v4l2_device[128];
    v4l2_capability v4l2_caps;
    struct dirent **v4l2_devices;

    int device_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
    if (device_count==0) {
        printf("no V4Linux2 devices found\n",device_count);
        return;
    }

    if (device_count==1) printf("1 V4Linux2 device found:\n",device_count);
    else printf("%d V4Linux2 devices found:\n",device_count);

    for (int i=0;i<device_count;i++) {
	sprintf(v4l2_device,"/dev/%s",v4l2_devices[i]->d_name);

	int fd = open(v4l2_device, O_RDONLY);
	if (fd < 0) continue;

    memset(&v4l2_caps, 0, sizeof(v4l2_capability));
    if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
		printf("%s oops\n",v4l2_device);
		close(fd);
    		continue;
	}

	if (v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        printf("\t%s: %s (%s)\n", v4l2_device, v4l2_caps.card, v4l2_caps.driver);

		for (int x=0;;x++) {
			struct v4l2_fmtdesc fmtdesc;
			memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
			fmtdesc.index = x;
			fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)) break;
			printf("\t\tformat: %s%s\n",  fmtdesc.description, fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED ? " (compressed)" : "");

			for (int y=0;;y++) {
				struct v4l2_frmsizeenum frmsize;
				memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
				frmsize.index = y;
				frmsize.pixel_format  = fmtdesc.pixelformat;
				if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&frmsize)) break;
				printf("\t\t\t %dx%d ",frmsize.discrete.width,frmsize.discrete.height);

				float last_fps=0.0f;
				for (int z=0;;z++) {
					struct v4l2_frmivalenum frmival;
					memset(&frmival, 0, sizeof(v4l2_frmivalenum));
					frmival.index = z;
					frmival.pixel_format = fmtdesc.pixelformat;
					frmival.width = frmsize.discrete.width;
					frmival.height = frmsize.discrete.height;
					if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMEINTERVALS,&frmival)) break;

					float frm_fps = frmival.discrete.denominator/(float)frmival.discrete.numerator;
					if(frm_fps==last_fps) break;
					last_fps=frm_fps;

					if (int(frm_fps) == frm_fps) printf("%d|",int(frm_fps));
					else printf("%'.1f|",frm_fps);
				}  printf("\b fps\n");
			}
		}
	}
	close(fd);
   }
}

bool V4Linux2Camera::findCamera() {

    readSettings();
    if (config.device<0) config.device=0;

    struct dirent **v4l2_devices;
    int device_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
    if (device_count==0) {
        printf("no V4Linux2 devices found\n",device_count);
        return false;
    } else if (device_count==1) printf("1 V4Linux2 device found\n",device_count);
    else printf("%d V4Linux2 devices found\n",device_count);

    char v4l2_device[128];
    sprintf(v4l2_device,"/dev/video%d",config.device);

    int fd = open(v4l2_device, O_RDWR);
    if (fd < 0) {
        printf("no V4Linux2 device found at %s\n", v4l2_device);
        return false;
    }

    memset(&v4l2_caps, 0, sizeof(v4l2_capability));
    if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
         printf("%s does not support video capture\n",v4l2_device);
        close(fd);
        return false;
    }

    if ((v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        printf("%s does not support video capture\n",v4l2_device);
        close(fd);
        return false;
    }

    if ((v4l2_caps.capabilities & V4L2_CAP_STREAMING) == 0) {
        printf("% does not support memory streaming capture\n",v4l2_device);
        close(fd);
        return false;
    }

    bool valid_compressed_format = false;
    bool valid_uncompressed_format = false;
    for (int i=0;;i++) {
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)) break;

        if ((fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED) && (fmtdesc.pixelformat==V4L2_PIX_FMT_MJPEG)) {
            valid_compressed_format = true;
            //printf("got compressed format\n");
        } else if (!(fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED) && ((fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_UYVY) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV420) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV410))) {
            valid_uncompressed_format = true;
            //printf("got uncompressed format\n");
        }
    }

    if ((valid_uncompressed_format==false) && (valid_compressed_format==false)) {
        printf("% does not support any valid pixel format\n",v4l2_device);
        close(fd);
        return false;
    }

    if ((config.compress==true) && (valid_compressed_format==false)) config.compress = false;
    if ((config.compress==false) && (valid_uncompressed_format==false)) config.compress = true;

    sprintf(cameraName, "%s (%s)", v4l2_caps.card, v4l2_caps.driver);
    cameraID = fd;
    return true;
}

bool V4Linux2Camera::startCamera() {

    for(unsigned int i = 0; i < v4l2_reqbuffers.count; ++i) {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory      = V4L2_MEMORY_MMAP;
        v4l2_buf.index       = i;

        if (-1 == ioctl (cameraID, VIDIOC_QBUF, &v4l2_buf)) {
            printf("Error queuing buffer: %s\n",  strerror(errno));
            exit(0);
        }
    }

    enum v4l2_buf_type type;
    memset(&type, 0, sizeof(v4l2_buf_type));
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cameraID, VIDIOC_STREAMON, &type) < 0) {
        printf("Cannot start camera: %s\n", strerror(errno));
        return false;
    }

    running = true;
    return true;
}

unsigned char* V4Linux2Camera::getFrame()  {

    if (cameraID==-1) return NULL;
    /*memset (&v4l2_buf, 0, sizeof (v4l2_buf));
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;*/

    if (ioctl(cameraID, VIDIOC_DQBUF, &v4l2_buf)<0) {
	//stopCamera();
	//closeCamera();
	running = false;
    	return NULL;
    }

    unsigned char *raw_buffer = (unsigned char*)buffers[v4l2_buf.index].start;
    if (raw_buffer==NULL) return NULL;

    if(colour) {
        // TODO portvideo RGB
        //ccvt_yuyv_rgb24(this->width, this->height, cam_buffer, buffer);
    } else {
        if (config.frame) {
            if (pixelformat==V4L2_PIX_FMT_YUYV) {

                unsigned char *src = raw_buffer;
                unsigned char *dest = crop_buffer;

                src += 2*(config.frame_yoff*cam_width);
                int xend = (cam_width-(frame_width+config.frame_xoff));

                for (int i=0;i<frame_height;i++) {

                    src +=  2*config.frame_xoff;
                    for (int j=frame_width/2;j>0;j--) {
                        *dest++ = *src++;
                        src++;
                        *dest++ = *src++;
                        src++;
                    }
                    src +=  2*xend;
                }
            }
            else if (pixelformat==V4L2_PIX_FMT_UYVY) {

                unsigned char *src = raw_buffer;
                unsigned char *dest = crop_buffer;

                src += 2*(config.frame_yoff*cam_width);
                int xend = (cam_width-(frame_width+config.frame_xoff));

                for (int i=0;i<frame_height;i++) {

                    src +=  2*config.frame_xoff;
                    for (int j=frame_width/2;j>0;j--) {
                        src++;
                        *dest++ = *src++;
                        src++;
                        *dest++ = *src++;
                    }
                    src +=  2*xend;
                }

            }
            else if (pixelformat==V4L2_PIX_FMT_YUV420) cropFrame(raw_buffer,crop_buffer);
            else if (pixelformat==V4L2_PIX_FMT_YUV410) cropFrame(raw_buffer,crop_buffer);
            else if (pixelformat==V4L2_PIX_FMT_MJPEG) {

                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cam_width, &cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cam_width, 0, cam_height, TJPF_GRAY, TJFLAG_FASTDCT);
                cropFrame(cam_buffer,crop_buffer);
            }
        } else {
            if (pixelformat==V4L2_PIX_FMT_YUYV) yuyv2gray(cam_width, cam_height, raw_buffer, cam_buffer);
            else if (pixelformat==V4L2_PIX_FMT_UYVY) uyvy2gray(cam_width, cam_height, raw_buffer, cam_buffer);
            else if (pixelformat==V4L2_PIX_FMT_YUV420) memcpy(cam_buffer,raw_buffer,cam_width*cam_height);
            else if (pixelformat==V4L2_PIX_FMT_YUV410) memcpy(cam_buffer,raw_buffer,cam_width*cam_height);
            else if (pixelformat==V4L2_PIX_FMT_MJPEG)  {

                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cam_width, &cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cam_width, 0, cam_height, TJPF_GRAY, TJFLAG_FASTDCT);
            }
        }
	}

    if (-1 == ioctl (cameraID, VIDIOC_QBUF, &v4l2_buf)) {
        printf("cannot unqueue buffer: %s\n", strerror(errno));
        return NULL;
    }

    if (config.frame) return crop_buffer;
    else return cam_buffer;
}

bool V4Linux2Camera::stopCamera() {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cameraID, VIDIOC_STREAMOFF, &type) < 0) {
        //printf("cannot stop camera: %s\n", strerror(errno));
        return false;
    }

    running = false;
    return true;
}

bool V4Linux2Camera::stillRunning() {
 	  return running;
}

bool V4Linux2Camera::resetCamera()
{
    return (stopCamera() && startCamera());
}


bool V4Linux2Camera::closeCamera() {

    if (cameraID >= 0) {
        if (buffers_initialized) unmapBuffers();
        close(cameraID);
        cameraID = -1;

        return true;
    } else return false;

}

bool V4Linux2Camera::requestBuffers() {
    // Request mmap buffers
    memset (&v4l2_reqbuffers, 0, sizeof (v4l2_reqbuffers));
    v4l2_reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffers.memory = V4L2_MEMORY_MMAP;
    v4l2_reqbuffers.count = nr_of_buffers;

    if (-1 == ioctl (cameraID, VIDIOC_REQBUFS, &v4l2_reqbuffers)) {
        if (errno == EINVAL)
            printf ("video capturing or mmap-streaming is not supported\n");
        else
            perror ("VIDIOC_REQBUFS");

        return false;
    }

    if (v4l2_reqbuffers.count < 1) {
        /* You may need to free the buffers here. */
        printf("Error requesting buffers.\n");
        return false;
    }

    return true;
}

bool V4Linux2Camera::mapBuffers() {

    for(unsigned int i=0; i<v4l2_reqbuffers.count; i++) {

        memset (&v4l2_buf, 0, sizeof (v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = i;

        if (-1 == ioctl (cameraID, VIDIOC_QUERYBUF, &v4l2_buf)) {
            printf("Unable to query buffer: %s\n",  strerror(errno));
            return false;
        }

        buffers[i].length = v4l2_buf.length; /* remember for munmap() */
        buffers[i].start = mmap (NULL, v4l2_buf.length,
              PROT_READ | PROT_WRITE, /* recommended */
              MAP_SHARED,             /* recommended */
              cameraID, v4l2_buf.m.offset);

        if (MAP_FAILED == buffers[i].start) {
            /* If you do not exit here you should unmap() and free()
            the buffers mapped so far. */
            printf("Unable to map buffer %d.\n",  i);
            return false;
        }
    }

    return true;
}

bool V4Linux2Camera::unmapBuffers() {
    for(unsigned int i=0; i<v4l2_reqbuffers.count; i++) {
        if (-1 == munmap(buffers[i].start, buffers[i].length)) {
            printf("Error unmapping buffer %d.\n", i);
            return false;
        }
    }
    return true;
}

bool V4Linux2Camera::setDefaultCameraSetting(int mode) {
	return false;
}

int V4Linux2Camera::getDefaultCameraSetting(int mode) {
	return INT_MIN;
}


bool V4Linux2Camera::getCameraSettingAuto(int mode) {
	return false;
}

bool V4Linux2Camera::setCameraSettingAuto(int mode, bool flag) {

	v4l2_ctrl.id = 0;

	switch (mode) {
		case GAIN:
			//v4l2_ctrl.id = V4L2_CID_AUTOGAIN;
			//v4l2_ctrl.value = (int)flag;
			break;
		case EXPOSURE:
			v4l2_ctrl.id = V4L2_CID_EXPOSURE_AUTO;
			if (flag==true) v4l2_ctrl.value = V4L2_EXPOSURE_AUTO;
			else {
				v4l2_ctrl.value = V4L2_EXPOSURE_MANUAL;
				v4l2_ctrl.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
				ioctl(cameraID, VIDIOC_S_CTRL, &v4l2_ctrl);
				v4l2_ctrl.id = V4L2_CID_EXPOSURE_AUTO;
			}
			break;
	}

	if (v4l2_ctrl.id==0) return false;

	if ((ioctl(cameraID, VIDIOC_S_CTRL, &v4l2_ctrl)) < 0) {
	    printf("Unable to set AUTO mode: %s\n" , strerror(errno));
	    return false;
	} return true;

}


bool V4Linux2Camera::setCameraSetting(int mode, int setting) {

	int current_setting = getCameraSetting(mode);
	if (setting==current_setting) return true;
	setCameraSettingAuto(mode,false);

	v4l2_ctrl.id = 0;
	v4l2_ctrl.value = setting;

	switch (mode) {
		case BRIGHTNESS: v4l2_ctrl.id = V4L2_CID_BRIGHTNESS; config.brightness = setting; break;
		case CONTRAST: v4l2_ctrl.id = V4L2_CID_CONTRAST; config.contrast = setting; break;
		case GAIN: v4l2_ctrl.id = V4L2_CID_GAIN; config.gain = setting; break;
		case EXPOSURE: v4l2_ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE; config.exposure = setting; break;
		case SHARPNESS: v4l2_ctrl.id = V4L2_CID_SHARPNESS; config.sharpness = setting; break;
		case GAMMA: v4l2_ctrl.id = V4L2_CID_GAMMA; config.gamma = setting; break;
	}

	if (v4l2_ctrl.id==0) return false;

	if ((ioctl(cameraID, VIDIOC_S_CTRL, &v4l2_ctrl)) < 0) {
	    return false;
	} return true;
}

int V4Linux2Camera::getCameraSetting(int mode) {

	v4l2_ctrl.id = 0;
	v4l2_ctrl.value = 0;

	switch (mode) {
		case BRIGHTNESS: v4l2_ctrl.id = V4L2_CID_BRIGHTNESS; break;
		case CONTRAST: v4l2_ctrl.id = V4L2_CID_CONTRAST; break;
		case GAIN: v4l2_ctrl.id = V4L2_CID_GAIN; break;
		case EXPOSURE: v4l2_ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
		case SHARPNESS: v4l2_ctrl.id = V4L2_CID_SHARPNESS; break;
		case GAMMA: v4l2_ctrl.id = V4L2_CID_GAMMA; break;
	} if (v4l2_ctrl.id==0) return false;

	if ((ioctl(cameraID, VIDIOC_G_CTRL, &v4l2_ctrl)) < 0) {
	    return 0;
	} return v4l2_ctrl.value;
}

int V4Linux2Camera::getMaxCameraSetting(int mode) {

 	v4l2_query.id = 0;

	switch (mode) {
		case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
		case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
		case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
		case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
		case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
		case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
	} if (v4l2_query.id==0) return 0;

	if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
		return 0;
    	} else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
		return 0;
    	} else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
		return 1;
    	} else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
		return v4l2_query.maximum;
    	}

	return 0;
}

int V4Linux2Camera::getMinCameraSetting(int mode) {
 	v4l2_query.id = 0;

	switch (mode) {
		case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
		case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
		case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
		case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
		case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
		case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
	} if (v4l2_query.id==0) return 0;

	if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
		return 0;
    	} else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
		return 0;
    	} else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
		return 1;
    	} else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
		return v4l2_query.minimum;
    	}

	return 0;
}

int V4Linux2Camera::getCameraSettingStep(int mode) {
 	v4l2_query.id = 0;

	switch (mode) {
		case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
		case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
		case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
		case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
		case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
		case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
	} if (v4l2_query.id==0) return 0;

	if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
		return 0;
    	} else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
		return 0;
    	} else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
		return 1;
    	} else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
		return v4l2_query.step;
    	}

	return 0;
}

