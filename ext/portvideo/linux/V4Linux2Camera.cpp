/* portVideo, a cross platform camera framework
 Copyright (C) 2005-2017 Martin Kaltenbrunner <martin@tuio.org>
 V4Linux2Camera initially contributed 2007 by Peter Eschler

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
#include "CameraTool.h"

unsigned int codec_table[] =  { 0, V4L2_PIX_FMT_GREY, 0,  V4L2_PIX_FMT_RGB24, 0, 0, 0, 0, 0, 0, V4L2_PIX_FMT_YUYV,
V4L2_PIX_FMT_UYVY, 0, V4L2_PIX_FMT_YUV444, V4L2_PIX_FMT_YUV420, V4L2_PIX_FMT_YUV410, V4L2_PIX_FMT_YVYU, 0, 0, 0, V4L2_PIX_FMT_JPEG,
V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_MPEG1, V4L2_PIX_FMT_MPEG2, V4L2_PIX_FMT_MPEG4, V4L2_PIX_FMT_H263, V4L2_PIX_FMT_H264, 0, 0, 0,  V4L2_PIX_FMT_DV, 0 };

V4Linux2Camera::V4Linux2Camera(CameraConfig *cam_cfg) : CameraEngine(cam_cfg)
{
    cam_buffer = NULL;
    frm_buffer = NULL;
    running = false;
    buffers_initialized = false;

    cam_cfg->driver = DRIVER_DEFAULT;
}

V4Linux2Camera::~V4Linux2Camera(void)
{
    CameraTool::saveSettings();
    if ((pixelformat == V4L2_PIX_FMT_MJPEG) || (pixelformat == V4L2_PIX_FMT_JPEG)) tjDestroy(_jpegDecompressor);
    if (cam_buffer!=NULL) delete []cam_buffer;
    cam_buffer = NULL;
    if (frm_buffer!=NULL) delete []frm_buffer;
    frm_buffer = NULL;
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

int V4Linux2Camera::getDeviceCount() {

	char v4l2_device[128];
	v4l2_capability v4l2_caps;
	memset(&v4l2_caps, 0, sizeof(v4l2_capability));
	struct dirent **v4l2_devices;

    	int dev_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
    	if (dev_count==0) return 0;

	int cam_count = 0;

	for (int i=0;i<dev_count;i++) {
        	sprintf(v4l2_device,"/dev/%s",v4l2_devices[i]->d_name);

        	int fd = open(v4l2_device, O_RDONLY);
        	if (fd < 0) continue;


       		if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
            		close(fd);
            		continue;
        	}

        	if ((v4l2_caps.device_caps & V4L2_CAP_VIDEO_CAPTURE) && (v4l2_caps.capabilities & V4L2_CAP_STREAMING)) {
        		cam_count++;
		}

		close(fd);
	}

	return cam_count;
}

std::vector<CameraConfig> V4Linux2Camera::getCameraConfigs(int dev_id) {

	std::vector<CameraConfig> cfg_list;

	char v4l2_device[128];
	v4l2_capability v4l2_caps;
	memset(&v4l2_caps, 0, sizeof(v4l2_capability));
	struct dirent **v4l2_devices;

	int dev_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
	if (dev_count==0) return cfg_list;

	for (int i=0;i<dev_count;i++) {
		int cam_id;
		sscanf(v4l2_devices[i]->d_name,"%*[^0-9]%d",&cam_id);
		if ((dev_id>=0) && (dev_id!=cam_id)) continue;
        	sprintf(v4l2_device,"/dev/%s",v4l2_devices[i]->d_name);

        	int fd = open(v4l2_device, O_RDONLY);
        	if (fd < 0) continue;

        	if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
        		close(fd);
                	continue;
            	}

    		if ((v4l2_caps.capabilities & V4L2_CAP_STREAMING) == 0) {
        		close(fd);
        		continue;
    		}

        	if (v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) {

			CameraConfig cam_cfg;
			CameraTool::initCameraConfig(&cam_cfg);

			cam_cfg.driver = DRIVER_DEFAULT;
			cam_cfg.device = cam_id;
			sprintf(cam_cfg.name, "%s (%s)", v4l2_caps.card, v4l2_caps.driver);

            		for (int x=0;;x++) {
                		struct v4l2_fmtdesc fmtdesc;
                		memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
                		fmtdesc.index = x;
                		fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                		if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)) break;

                        	cam_cfg.cam_format = FORMAT_UNKNOWN;
                        	for (int i=FORMAT_MAX;i>0;i--) {
                            		if (fmtdesc.pixelformat == codec_table[i]) {
                                		cam_cfg.cam_format = i;
                                		break;
                            		}
                        	} if (cam_cfg.cam_format == FORMAT_UNKNOWN) continue;

				std::vector<CameraConfig> tmp_list;

                		for (int y=0;;y++) {
                    			struct v4l2_frmsizeenum frmsize;
                    			memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
                    			frmsize.index = y;
                    			frmsize.pixel_format  = fmtdesc.pixelformat;
                    			if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&frmsize)) break;

					if (frmsize.type==V4L2_FRMSIZE_TYPE_DISCRETE) {
                    				cam_cfg.cam_width = frmsize.discrete.width;
                               			cam_cfg.cam_height = frmsize.discrete.height;

        	            			float last_fps=0.0f;
	                    			for (int z=0;;z++) {
                	        			struct v4l2_frmivalenum frmival;
                       					memset(&frmival, 0, sizeof(v4l2_frmivalenum));
                        				frmival.index = z;
                        				frmival.pixel_format = fmtdesc.pixelformat;
                     	  				frmival.width = cam_cfg.cam_width;
                       					frmival.height = cam_cfg.cam_height;
                       					if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMEINTERVALS,&frmival)) break;

                        				float frm_fps = roundf((frmival.discrete.denominator/(float)frmival.discrete.numerator)*10)/10.0f;
                       					if(frm_fps==last_fps) break;
                        				last_fps=frm_fps;

                        				cam_cfg.cam_fps = frm_fps;
							tmp_list.push_back(cam_cfg);
						}

					} else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {

						unsigned int step_w = frmsize.stepwise.step_width;
						unsigned int step_h = frmsize.stepwise.step_height;

						if ((frmsize.stepwise.max_width != frmsize.stepwise.max_height) && (step_w == step_h)) {
							float ratio = frmsize.stepwise.max_width/(float)frmsize.stepwise.max_height;
							if (ratio == 4.0f/3.0f) {
								step_w *= 16;
								step_h *= 12;
							} else if (ratio == 16.0f/9.0f) {
								step_w *= 16;
								step_h *= 9;
							}
						}

						unsigned int h = frmsize.stepwise.max_height;
						for (unsigned int w=frmsize.stepwise.max_width;w!=frmsize.stepwise.min_width;w-=step_w) {

							cam_cfg.cam_width = w;
                               				cam_cfg.cam_height = h;

                	        			struct v4l2_frmivalenum frmival;
                       					memset(&frmival, 0, sizeof(v4l2_frmivalenum));
                        				frmival.index = 0;
                        				frmival.pixel_format = fmtdesc.pixelformat;
                     	  				frmival.width = cam_cfg.cam_width;
                       					frmival.height = cam_cfg.cam_height;
                       					if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMEINTERVALS,&frmival)) break;

                        				cam_cfg.cam_fps = roundf((frmival.discrete.denominator/(float)frmival.discrete.numerator)*10)/10.0f;
							tmp_list.push_back(cam_cfg);

							h-=step_h;
						}
					}
                		}

				std::sort(tmp_list.begin(), tmp_list.end());
				cfg_list.insert( cfg_list.end(), tmp_list.begin(), tmp_list.end() );
            		}
        	}
        	close(fd);
	}

	return cfg_list;
}

CameraEngine* V4Linux2Camera::getCamera(CameraConfig *cam_cfg) {

	int cam_count = getDeviceCount();
	if (cam_count==0) return NULL;

	if ((cam_cfg->device==SETTING_MIN) || (cam_cfg->device==SETTING_DEFAULT)) cam_cfg->device=0;
	else if (cam_cfg->device==SETTING_MAX) cam_cfg->device=cam_count-1;

	std::vector<CameraConfig> cfg_list = V4Linux2Camera::getCameraConfigs(cam_cfg->device);
	if (cfg_list.size()==0) return NULL;

    	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);

	if (cam_cfg->force) return new V4Linux2Camera(cam_cfg);

    	for (unsigned int i=0;i<cfg_list.size();i++) {

        	if (cam_cfg->cam_format != cfg_list[i].cam_format) continue;
        	if ((cam_cfg->cam_width >=0) && (cam_cfg->cam_width != cfg_list[i].cam_width)) continue;
        	if ((cam_cfg->cam_height >=0) && (cam_cfg->cam_height != cfg_list[i].cam_height)) continue;
        	if ((cam_cfg->cam_fps >=0) && (cam_cfg->cam_fps != cfg_list[i].cam_fps)) continue;

	        return new V4Linux2Camera(cam_cfg);
    	}

	return NULL;
}

bool V4Linux2Camera::initCamera() {

    struct dirent **v4l2_devices;
    int dev_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
    if ((dev_count == 0) || (cfg->device<0)) return false;

    char v4l2_device[128];
    sprintf(v4l2_device,"/dev/video%d",cfg->device);

    dev_handle = open(v4l2_device, O_RDWR);
    if (dev_handle < 0) return false;

    memset(&v4l2_caps, 0, sizeof(v4l2_capability));
    if (ioctl(dev_handle, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
        close(dev_handle);
        return false;
    }

    if ((v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        close(dev_handle);
        return false;
    }

    if ((v4l2_caps.capabilities & V4L2_CAP_STREAMING) == 0) {
        close(dev_handle);
        return false;
    }

    sprintf(cfg->name, "%s (%s)", v4l2_caps.card, v4l2_caps.driver);

    std::vector<CameraConfig> cfg_list = V4Linux2Camera::getCameraConfigs(cfg->device);
    if (cfg->cam_format==FORMAT_UNKNOWN) cfg->cam_format = cfg_list[0].cam_format;
    setMinMaxConfig(cfg,cfg_list);

    pixelformat=0;
    for (int i=0;;i++) {
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(dev_handle,VIDIOC_ENUM_FMT,&fmtdesc)) break;

        for (int i=FORMAT_MAX;i>0;i--) {
            if (fmtdesc.pixelformat == codec_table[i]) {
                if (cfg->cam_format==i) {
					pixelformat = fmtdesc.pixelformat;
					break;
				}
            }
        }

        if(pixelformat) break;
    }

    if (!pixelformat) {
        printf("%s does not support any valid pixel format\n",v4l2_device);
        close(dev_handle);
        return false;
    }

    cfg->cam_format = FORMAT_UNKNOWN;
    for (int i=FORMAT_MAX;i>0;i--) {
        if (pixelformat == codec_table[i]) {
            cfg->cam_format = i;
            break;
        }
    }

    // try to set the desired format
    memset(&v4l2_form, 0, sizeof(v4l2_format));
    v4l2_form.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_form.fmt.pix.pixelformat = pixelformat;
    v4l2_form.fmt.pix.width  = cfg->cam_width;
    v4l2_form.fmt.pix.height = cfg->cam_height;

    if (-1 == ioctl (dev_handle, VIDIOC_S_FMT, &v4l2_form)) {
        printf("error setting pixel format: %s\n" , strerror(errno));
        return false;
    }

    // try to set the desired fps
    v4l2_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_parm.parm.capture.timeperframe.numerator = 1;
    v4l2_parm.parm.capture.timeperframe.denominator = int(cfg->cam_fps);

    if(-1 == ioctl (dev_handle, VIDIOC_S_PARM, &v4l2_parm)) {
        printf("error setting fps: %s\n", strerror(errno));
        //return false;
    }

    // use the settings we got from the driver
    pixelformat = v4l2_form.fmt.pix.pixelformat;
    cfg->cam_width = v4l2_form.fmt.pix.width;
    cfg->cam_height = v4l2_form.fmt.pix.height;
    cfg->cam_fps = roundf((v4l2_parm.parm.capture.timeperframe.denominator/(float)v4l2_parm.parm.capture.timeperframe.numerator)*10)/10.0f;

    if ((pixelformat == V4L2_PIX_FMT_MJPEG) || (pixelformat == V4L2_PIX_FMT_JPEG)) _jpegDecompressor = tjInitDecompress();

    if (!requestBuffers()) {
        printf("Error requesting buffers.\n");
        return false;
    }

    if (!mapBuffers()) {
        printf("Unable to mmap buffers.\n");
        return false;
    }

    setupFrame();
    if (cfg->frame) frm_buffer = new unsigned char[cfg->frame_width*cfg->frame_height*cfg->buf_format];
    cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];
    buffers_initialized = true;
    return true;
}

bool V4Linux2Camera::startCamera() {

    for(unsigned int i = 0; i < v4l2_reqbuffers.count; ++i) {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory      = V4L2_MEMORY_MMAP;
        v4l2_buf.index       = i;

        if (-1 == ioctl (dev_handle, VIDIOC_QBUF, &v4l2_buf)) {
            printf("Error queuing buffer: %s\n",  strerror(errno));
            exit(0);
        }
    }

    enum v4l2_buf_type type;
    memset(&type, 0, sizeof(v4l2_buf_type));
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(dev_handle, VIDIOC_STREAMON, &type) < 0) {
        printf("Cannot start camera: %s\n", strerror(errno));
        return false;
    }

    applyCameraSettings();
    running = true;
    return true;
}

unsigned char* V4Linux2Camera::getFrame()  {

    if (dev_handle<0) return NULL;

    if (ioctl(dev_handle, VIDIOC_DQBUF, &v4l2_buf)<0) {
        running = false;
        return NULL;
    }

    unsigned char *raw_buffer = (unsigned char*)buffers[v4l2_buf.index].start;
    if (raw_buffer==NULL) return NULL;

    if(cfg->color) {
        if (cfg->frame) {
         if (pixelformat==V4L2_PIX_FMT_YUYV)
            crop_yuyv2rgb(cfg->cam_width,raw_buffer,frm_buffer);
         else if (pixelformat==V4L2_PIX_FMT_UYVY)
            crop_uyvy2rgb(cfg->cam_width,raw_buffer,frm_buffer);
         else if (pixelformat==V4L2_PIX_FMT_YUV420) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_YUV410) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_GREY)
            crop_gray2rgb(cfg->cam_width,raw_buffer, frm_buffer);
         else if ((pixelformat == V4L2_PIX_FMT_MJPEG) || (pixelformat == V4L2_PIX_FMT_JPEG)) {
                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cfg->cam_width, &cfg->cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cfg->cam_width, 0, cfg->cam_height, TJPF_RGB, TJFLAG_FASTDCT);
                crop(cfg->cam_width, cfg->cam_height,cam_buffer,frm_buffer,3);
         }

        } else {
         if (pixelformat==V4L2_PIX_FMT_YUYV)
            yuyv2rgb(cfg->cam_width,cfg->cam_height,raw_buffer,cam_buffer);
         else if (pixelformat==V4L2_PIX_FMT_UYVY)
            uyvy2rgb(cfg->cam_width,cfg->cam_height,raw_buffer,cam_buffer);
         else if (pixelformat==V4L2_PIX_FMT_YUV420) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_YUV410) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_GREY)
            gray2rgb(cfg->cam_width,cfg->cam_height,raw_buffer,cam_buffer);
         else if ((pixelformat == V4L2_PIX_FMT_MJPEG) || (pixelformat == V4L2_PIX_FMT_JPEG)) {
                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cfg->cam_width, &cfg->cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cfg->cam_width, 0, cfg->cam_height, TJPF_RGB, TJFLAG_FASTDCT);
         }

        }

    } else {
        if (cfg->frame) {
            if (pixelformat==V4L2_PIX_FMT_YUYV)
                crop_yuyv2gray(cfg->cam_width,raw_buffer,frm_buffer);
            else if (pixelformat==V4L2_PIX_FMT_UYVY)
                crop_uyvy2gray(cfg->cam_width,raw_buffer,frm_buffer);
            else if (pixelformat==V4L2_PIX_FMT_YUV420)
                crop(cfg->cam_width, cfg->cam_height,raw_buffer,frm_buffer,1);
            else if (pixelformat==V4L2_PIX_FMT_YUV410)
                crop(cfg->cam_width, cfg->cam_height,raw_buffer,frm_buffer,1);
            else if (pixelformat==V4L2_PIX_FMT_GREY)
                crop(cfg->cam_width, cfg->cam_height,raw_buffer,frm_buffer,1);
            else if ((pixelformat == V4L2_PIX_FMT_MJPEG) || (pixelformat == V4L2_PIX_FMT_JPEG)) {

                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cfg->cam_width, &cfg->cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cfg->cam_width, 0, cfg->cam_height, TJPF_GRAY, TJFLAG_FASTDCT);
                crop(cfg->cam_width, cfg->cam_height,cam_buffer,frm_buffer,1);
            }
        } else {
            if (pixelformat==V4L2_PIX_FMT_YUYV) yuyv2gray(cfg->cam_width, cfg->cam_height, raw_buffer, cam_buffer);
            else if (pixelformat==V4L2_PIX_FMT_UYVY) uyvy2gray(cfg->cam_width, cfg->cam_height, raw_buffer, cam_buffer);
            else if (pixelformat==V4L2_PIX_FMT_YUV420) memcpy(cam_buffer,raw_buffer,cfg->cam_width*cfg->cam_height);
            else if (pixelformat==V4L2_PIX_FMT_YUV410) memcpy(cam_buffer,raw_buffer,cfg->cam_width*cfg->cam_height);
            //else if (pixelformat==V4L2_PIX_FMT_GREY) memcpy(cam_buffer,raw_buffer,cam_width*cam_height);
            else if ((pixelformat == V4L2_PIX_FMT_MJPEG) || (pixelformat == V4L2_PIX_FMT_JPEG)) {

                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cfg->cam_width, &cfg->cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cfg->cam_width, 0, cfg->cam_height, TJPF_GRAY, TJFLAG_FASTDCT);
            }
        }
    }

    if (-1 == ioctl (dev_handle, VIDIOC_QBUF, &v4l2_buf)) {
        printf("cannot unqueue buffer: %s\n", strerror(errno));
        return NULL;
    }

    if (cfg->frame) return frm_buffer;
    else if ((!cfg->color) && (pixelformat==V4L2_PIX_FMT_GREY)) return raw_buffer;
    else return cam_buffer;
}

bool V4Linux2Camera::stopCamera() {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(dev_handle, VIDIOC_STREAMOFF, &type) < 0) {
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

    if (dev_handle >= 0) {
	updateSettings();
        if (buffers_initialized) unmapBuffers();
        close(dev_handle);

        return true;
    } else return false;

}

bool V4Linux2Camera::requestBuffers() {
    // Request mmap buffers
    memset (&v4l2_reqbuffers, 0, sizeof (v4l2_reqbuffers));
    v4l2_reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffers.memory = V4L2_MEMORY_MMAP;
    v4l2_reqbuffers.count = nr_of_buffers;

    if (-1 == ioctl (dev_handle, VIDIOC_REQBUFS, &v4l2_reqbuffers)) {
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

        if (-1 == ioctl (dev_handle, VIDIOC_QUERYBUF, &v4l2_buf)) {
            printf("Unable to query buffer: %s\n",  strerror(errno));
            return false;
        }

        buffers[i].length = v4l2_buf.length; /* remember for munmap() */
        buffers[i].start = mmap (NULL, v4l2_buf.length,
                                 PROT_READ | PROT_WRITE, /* recommended */
                                 MAP_SHARED,             /* recommended */
                                 dev_handle, v4l2_buf.m.offset);

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

bool V4Linux2Camera::hasCameraSettingAuto(int mode) {

    v4l2_queryctrl v4l2_query;
    memset(&v4l2_query,0,sizeof(v4l2_queryctrl));

    switch (mode) {
        case GAIN: v4l2_query.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_AUTO; break;
        case WHITE: v4l2_query.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_AUTO; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE_AUTO; break;
        default: return false;
    }

    if (( ioctl(dev_handle, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return false;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return false;
    } else return true;

}

bool V4Linux2Camera::getCameraSettingAuto(int mode) {

    if (!hasCameraSetting(mode)) return false;

    switch (mode) {
        case GAIN: return getCameraSetting(AUTO_GAIN);
        case EXPOSURE:

	   struct v4l2_ext_control v4l2_auto_ctrl[1];
           v4l2_auto_ctrl[0].id = V4L2_CID_EXPOSURE_AUTO;
	   struct v4l2_ext_controls v4l2_auto_ctrls;
	   v4l2_auto_ctrls.count = 1;
	   v4l2_auto_ctrls.controls = v4l2_auto_ctrl;

	   if ((ioctl(dev_handle, VIDIOC_G_EXT_CTRLS, &v4l2_auto_ctrls)) < 0) {
		printf("Unable to get AUTO mode: %s\n" , strerror(errno));
        	return false;
    	   } else return (v4l2_auto_ctrl[0].value!=1);

        case WHITE: return getCameraSetting(AUTO_WHITE);
        case COLOR_HUE: return getCameraSetting(AUTO_HUE);
        default: return false;
    }

    return false;
}

bool V4Linux2Camera::setCameraSettingAuto(int mode, bool flag) {

    if (!hasCameraSetting(mode)) return false;

    switch (mode) {
        case GAIN:
           return(setCameraSetting(AUTO_GAIN,flag));
        case EXPOSURE:

	    struct v4l2_ext_control v4l2_auto_ctrl[1];
            v4l2_auto_ctrl[0].id = V4L2_CID_EXPOSURE_AUTO;
            if (flag==true) v4l2_auto_ctrl[0].value = V4L2_EXPOSURE_APERTURE_PRIORITY;
            else v4l2_auto_ctrl[0].value = V4L2_EXPOSURE_MANUAL;

	    struct v4l2_ext_controls v4l2_auto_ctrls;
	    v4l2_auto_ctrls.count = 1;
	    v4l2_auto_ctrls.controls = v4l2_auto_ctrl;

	    if ((ioctl(dev_handle, VIDIOC_S_EXT_CTRLS, &v4l2_auto_ctrls)) < 0) {

		if (flag==true) {
			v4l2_auto_ctrl[0].value = V4L2_EXPOSURE_AUTO;
			if ((ioctl(dev_handle, VIDIOC_S_EXT_CTRLS, &v4l2_auto_ctrls)) < 0) {
	        		printf("Unable to set AUTO mode: %s\n",strerror(errno));
	        		return false;
			}
		} else {
	        	printf("Unable to set AUTO mode: %s\n",strerror(errno));
	        	return false;
		}
	    }

	    if (!flag) setDefaultCameraSetting(EXPOSURE);
	    return true;
        case WHITE:
            return(setCameraSetting(AUTO_WHITE,flag));
        case COLOR_HUE:
            return(setCameraSetting(AUTO_HUE,flag));
        default: return false;
    }

    return false;
}

bool V4Linux2Camera::hasCameraSetting(int mode) {

    struct v4l2_queryctrl v4l2_query;
    memset(&v4l2_query,0,sizeof(v4l2_queryctrl));

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return hasCameraSettingAuto(GAIN);
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return hasCameraSettingAuto(EXPOSURE);
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return hasCameraSettingAuto(WHITE);
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return hasCameraSettingAuto(FOCUS);
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case SATURATION: v4l2_query.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return hasCameraSettingAuto(COLOR_HUE);
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return false;
    }

    if (( ioctl(dev_handle, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return false;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return false;
    } return true;
}

bool V4Linux2Camera::setCameraSetting(int mode, int setting) {

    if (!hasCameraSetting(mode)) return false;
    struct v4l2_control v4l2_ctrl;
    memset(&v4l2_ctrl,0,sizeof(v4l2_control));
    v4l2_ctrl.value = setting;

    switch (mode) {
        case BRIGHTNESS: v4l2_ctrl.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_ctrl.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_ctrl.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_ctrl.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN:  v4l2_ctrl.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return setCameraSettingAuto(EXPOSURE,setting);
        case WHITE: v4l2_ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: v4l2_ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case FOCUS: v4l2_ctrl.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return setCameraSettingAuto(FOCUS,setting);
        case BACKLIGHT: v4l2_ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case GAMMA: v4l2_ctrl.id = V4L2_CID_GAMMA; break;
        case SATURATION: v4l2_ctrl.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_ctrl.id = V4L2_CID_HUE; break;
        case AUTO_HUE: v4l2_ctrl.id = V4L2_CID_HUE_AUTO; break;
        case COLOR_RED: v4l2_ctrl.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_ctrl.id = V4L2_CID_BLUE_BALANCE; break;
        default: return false;
    }

    if ((ioctl(dev_handle, VIDIOC_S_CTRL, &v4l2_ctrl)) < 0) {
        //printf("Unable to set value: %s\n", strerror(errno));
        return false;
    } return true;
}

int V4Linux2Camera::getCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    v4l2_control v4l2_ctrl;
    memset(&v4l2_ctrl,0,sizeof(v4l2_control));

    switch (mode) {
        case BRIGHTNESS: v4l2_ctrl.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_ctrl.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_ctrl.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_ctrl.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: v4l2_ctrl.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return getCameraSettingAuto(EXPOSURE); break;
        case WHITE: v4l2_ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: v4l2_ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case FOCUS: v4l2_ctrl.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return getCameraSettingAuto(FOCUS);
        case BACKLIGHT: v4l2_ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case GAMMA: v4l2_ctrl.id = V4L2_CID_GAMMA; break;
        case SATURATION: v4l2_ctrl.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_ctrl.id = V4L2_CID_HUE; break;
        case AUTO_HUE: v4l2_ctrl.id = V4L2_CID_HUE_AUTO; break;
        case COLOR_RED: v4l2_ctrl.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_ctrl.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if ((ioctl(dev_handle, VIDIOC_G_CTRL, &v4l2_ctrl)) < 0) {
        return 0;
    } else return v4l2_ctrl.value;
}

bool V4Linux2Camera::setDefaultCameraSetting(int mode) {
    return setCameraSetting(mode,getDefaultCameraSetting(mode));
}

int V4Linux2Camera::getDefaultCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    v4l2_queryctrl v4l2_query;
    memset(&v4l2_query,0,sizeof(v4l2_queryctrl));

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: v4l2_query.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_AUTO; break;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: v4l2_query.id = V4L2_CID_FOCUS_AUTO; break;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: v4l2_query.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case SATURATION: v4l2_query.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: v4l2_query.id = V4L2_CID_HUE_AUTO; break;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(dev_handle, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return 0;
    } else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
        return v4l2_query.default_value;
    }

    return 0;
}

int V4Linux2Camera::getMaxCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    v4l2_queryctrl v4l2_query;
    memset(&v4l2_query,0,sizeof(v4l2_queryctrl));

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return 1;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return 1;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return 1;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return 1;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case SATURATION: v4l2_query.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return 1;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(dev_handle, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
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

    if (!hasCameraSetting(mode)) return 0;
    struct v4l2_queryctrl v4l2_query;
    memset(&v4l2_query,0,sizeof(v4l2_queryctrl));

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return 0;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return 0;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return 0;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return 0;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case SATURATION: v4l2_query.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return 0;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(dev_handle, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return 0;
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
        return 0;
    } else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
        return v4l2_query.minimum;
    }

    return 0;
}

int V4Linux2Camera::getCameraSettingStep(int mode) {

    struct v4l2_queryctrl v4l2_query = {0};
    memset(&v4l2_query,0,sizeof(v4l2_queryctrl));

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return 1;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return 1;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return 1;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return 1;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case SATURATION: v4l2_query.id = V4L2_CID_SATURATION; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return 1;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(dev_handle, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
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

