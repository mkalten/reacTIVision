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

#include "DC1394Camera.h"
#include "CameraTool.h"

DC1394Camera::DC1394Camera(CameraConfig* cam_cfg):CameraEngine(cam_cfg)
{
	cam_buffer = NULL;
	camera=NULL;
	list=NULL;
	d=NULL;

	running=false;
	timeout= 5000;
	lost_frames=0;
	
	cam_cfg->driver = DRIVER_DC1394;
}

DC1394Camera::~DC1394Camera()
{
        CameraTool::saveSettings();
	if (cam_buffer!=NULL) delete []cam_buffer;
}

int DC1394Camera::getDeviceCount() {
	
	dc1394_t *d = dc1394_new();
	dc1394camera_list_t *list;
	
	if (dc1394_camera_enumerate (d, &list) != DC1394_SUCCESS) {
		dc1394_free(d);
		return 0;
	}
	
	int dev_count = list->num;
	dc1394_camera_free_list (list);
	dc1394_free(d);
	
	return dev_count;
}

std::vector<CameraConfig> DC1394Camera::getCameraConfigs(int dev_id) {
    std::vector<CameraConfig> cfg_list;
	
	dc1394_t *d = dc1394_new();
	dc1394camera_list_t *list;
	
	float framerate_table[] = { 1.875f,3.75f,7.5f,15.0f,30.0f,60.0f,120.0f,240.0f };
	int width_table[] = {160,320,640,640,640,640,640,800,800,800,1024,1024,1024,800,1024,1280,1280,1280,1600,1600,1600,1280,1600 };
	int height_table[] = {120,240,480,480,480,480,480,600,600,600,768,768,768,600,768,960,960,960,1200,1200,1200,960,1200 };
	int codec_table[] = {FORMAT_GRAY,FORMAT_YUV411,FORMAT_YUYV,FORMAT_YUV444,FORMAT_RGB,FORMAT_GRAY16,FORMAT_RGB16,FORMAT_GRAY16S,FORMAT_RGB16S,FORMAT_RAW8,FORMAT_RAW16};
	
	if (dc1394_camera_enumerate (d, &list) != DC1394_SUCCESS) {
		dc1394_free(d);
		return cfg_list;
	}
	
	if (list->num == 0) {
		dc1394_camera_free_list(list);
		dc1394_free(d);
		return cfg_list;
	}
	
	for (unsigned int i=0;i<list->num;i++) {
		if ((dev_id>=0) && (dev_id!=(int)i)) continue;
		dc1394camera_t *camera = dc1394_camera_new (d, list->ids[i].guid);
		if (!camera) continue;
		
		CameraConfig cam_cfg;
		CameraTool::initCameraConfig(&cam_cfg);
		
		cam_cfg.driver = DRIVER_DC1394;
		cam_cfg.device = i;
		sprintf(cam_cfg.name,"%s",camera->model);
		
		dc1394video_modes_t video_modes;
		
		if (dc1394_video_get_supported_modes(camera,&video_modes)!=DC1394_SUCCESS) {
			dc1394_camera_free(camera);
			continue;
		}
		
		for (unsigned int i=0;i<video_modes.num;i++) {
			
			if(video_modes.modes[i]>=DC1394_VIDEO_MODE_FORMAT7_0) {
				dc1394color_coding_t coding;
				dc1394_get_color_coding_from_video_mode(camera,video_modes.modes[i], &coding);
				cam_cfg.cam_format = codec_table[coding-DC1394_COLOR_CODING_MIN];
				
				uint32_t F7_xmax, F7_ymax;
				dc1394_format7_get_max_image_size(camera, video_modes.modes[i], &F7_xmax, &F7_ymax);
				
				cam_cfg.cam_width = F7_xmax;
				cam_cfg.cam_height = F7_ymax;
				cam_cfg.frame_width = F7_xmax;
				cam_cfg.frame_height = F7_ymax;
				cam_cfg.frame_xoff = 0;
				cam_cfg.frame_yoff = 0;
				cam_cfg.frame = true;
				cam_cfg.frame_mode = (int)(video_modes.modes[i] - DC1394_VIDEO_MODE_FORMAT7_0);
				cfg_list.push_back(cam_cfg);
				
			} else {
				
				dc1394color_coding_t coding;
				dc1394_get_color_coding_from_video_mode(camera,video_modes.modes[i], &coding);
				cam_cfg.cam_format = codec_table[coding-DC1394_COLOR_CODING_MIN];
				
				unsigned int width,height;
				dc1394_get_image_size_from_video_mode(camera, video_modes.modes[i], &width, &height);
				cam_cfg.cam_width = width_table[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN];
				cam_cfg.cam_height = height_table[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN];
				
				dc1394framerates_t framerates;
				dc1394_video_get_supported_framerates(camera,video_modes.modes[i],&framerates);
				for (unsigned int j=0;j<framerates.num;j++) {
					cam_cfg.cam_fps = framerate_table[framerates.framerates[j]-DC1394_FRAMERATE_MIN];
					cfg_list.push_back(cam_cfg);
				}
				
				cam_cfg.frame_xoff = 0;
				cam_cfg.frame_yoff = 0;
				cam_cfg.frame = false;
				cam_cfg.frame_mode = -1;
			}
		}
		
		dc1394_camera_free(camera);
	}
	
	dc1394_camera_free_list(list);
	dc1394_free(d);
	
    return cfg_list;
}

CameraEngine* DC1394Camera::getCamera(CameraConfig *cam_cfg) {

	dc1394_t *d = dc1394_new();
	dc1394camera_list_t *list;
	
	if (dc1394_camera_enumerate (d, &list) != DC1394_SUCCESS) {
		dc1394_free(d);
		return NULL;
	}
	
	if (list->num == 0) {
		dc1394_free(d);
		dc1394_camera_free_list(list);
		return NULL;
	}
	
	dc1394_free(d);
	dc1394_camera_free_list (list);
	
	if ((cam_cfg->device==SETTING_MIN) || (cam_cfg->device==SETTING_DEFAULT)) cam_cfg->device=0;
	else if (cam_cfg->device==SETTING_MAX) cam_cfg->device=list->num-1;
	
	std::vector<CameraConfig> cfg_list = DC1394Camera::getCameraConfigs(cam_cfg->device);
	if (cam_cfg->cam_format==FORMAT_UNKNOWN) cam_cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cam_cfg,cfg_list);
	
	if (cam_cfg->force) return new DC1394Camera(cam_cfg);
	
	for (unsigned int i=0;i<cfg_list.size();i++) {
			
		if (cam_cfg->frame_mode<0) cam_cfg->frame=false;
		if ((cam_cfg->frame) && (cam_cfg->frame_mode != cfg_list[i].frame_mode)) continue;
		if ((cam_cfg->cam_width >=0) && (cam_cfg->cam_width != cfg_list[i].cam_width)) continue;
		if ((cam_cfg->cam_height >=0) && (cam_cfg->cam_height != cfg_list[i].cam_height)) continue;
		if ((cam_cfg->cam_fps >=0) && (cam_cfg->cam_fps != cfg_list[i].cam_fps)) continue;

		return new DC1394Camera(cam_cfg);
	}
	
	return NULL;
}

bool DC1394Camera::initCamera() {

	d = dc1394_new();
	
	if (dc1394_camera_enumerate (d, &list) != DC1394_SUCCESS) {
		dc1394_free(d);
		return false;
	}
	
	if ((list->num == 0) || (cfg->device<0) || (cfg->device>=(int)list->num)) {
		dc1394_free(d);
		return false;
	}

	camera = dc1394_camera_new (d, list->ids[cfg->device].guid);
	if (!camera) {
		fprintf (stderr, "Failed to initialize DC1394 camera\n");
		return false;
	}
	dc1394_camera_free_list (list);

	sprintf(cfg->name,"%s", camera->model);
	
	std::vector<CameraConfig> cfg_list = getCameraConfigs(cfg->device);
	if (cfg->cam_format==FORMAT_UNKNOWN) cfg->cam_format = cfg_list[0].cam_format;
	setMinMaxConfig(cfg, cfg_list);

	dc1394framerates_t framerates;
	dc1394video_modes_t video_modes;
	dc1394framerate_t framerate;
	dc1394video_mode_t video_mode;
	dc1394color_coding_t color_coding;
	
	int color_table[] = {FORMAT_GRAY,FORMAT_YUV411,FORMAT_YUYV,FORMAT_YUV444,FORMAT_RGB,FORMAT_GRAY16,FORMAT_RGB16,FORMAT_GRAY16S,FORMAT_RGB16S,FORMAT_RAW8,FORMAT_RAW16};

	int format = -1;
	for (int i=0;i<12;i++) {
		if (cfg->cam_format == color_table[i]) {
			format = i;
			color_coding = (dc1394color_coding_t)(DC1394_COLOR_CODING_MIN + format);
			break;
		}
	}
	if (format<0) return false;

	float framerate_table[] = { 1.875f,3.75f,7.5f,15.0f,30.0f,60.0f,120.0f,240.0f };
	int width_table[] = {160,320,640,640,640,640,640,800,800,800,1024,1024,1024,800,1024,1280,1280,1280,1600,1600,1600,1280,1600 };
	int height_table[] = {120,240,480,480,480,480,480,600,600,600,768,768,768,600,768,960,960,960,1200,1200,1200,960,1200 };

    bool force_FW800 = false;
    bool force_FW400 = false;

    if (std::string(camera->model).find("Firefly MV FFMV")!=std::string::npos) force_FW400 = true;
    if (std::string(camera->model).find("Chameleon")!=std::string::npos) force_FW400 = true;

	if ( ((!force_FW400) && (camera->bmode_capable==DC1394_TRUE)) || (force_FW800))  {
		// set operation mode legacy or 1394B
		dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
        dc1394operation_mode_t operation_mode;
		dc1394_video_get_operation_mode(camera, &operation_mode);

		if (operation_mode == DC1394_OPERATION_MODE_1394B) {
				dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
		} else {
			dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
		}
	} else {
		dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_LEGACY);
		dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
	}

	if (cfg->frame==true) {
		if (cfg->frame_mode<0) cfg->frame_mode=0;
		video_mode = (dc1394video_mode_t)(DC1394_VIDEO_MODE_FORMAT7_0 + cfg->frame_mode);
		dc1394format7mode_t f7_mode;
		if ( dc1394_format7_get_mode_info(camera,video_mode, &f7_mode) !=DC1394_SUCCESS) {
			std::cerr << "FORMAT7 not supported" << std::endl;
			return false;
		}

		uint32_t F7_xmax, F7_ymax = 0;
 		uint32_t F7_bmin, F7_bmax = 0;

		dc1394_format7_get_max_image_size(camera, video_mode, &F7_xmax, &F7_ymax);
		dc1394_format7_get_packet_parameters(camera, video_mode, &F7_bmin, &F7_bmax);

		if (cfg->frame_xoff==SETTING_MAX) cfg->frame_xoff = F7_xmax;
		if (cfg->frame_yoff==SETTING_MAX) cfg->frame_yoff = F7_ymax;

		uint32_t F7_x = cfg->frame_xoff;
		uint32_t F7_y = cfg->frame_yoff;

		uint32_t F7_w = cfg->frame_width;
		if (cfg->frame_width==SETTING_MAX) F7_w = F7_xmax;
		if (F7_w>(F7_xmax-F7_x)) F7_w=F7_xmax-F7_x;
        if (F7_w%2!=0) F7_w--; // only even numbers

		uint32_t F7_h = cfg->frame_height;
		if (cfg->frame_height==SETTING_MAX) F7_h = F7_ymax;
		if (F7_h>(F7_ymax-F7_y)) F7_h=F7_ymax-F7_y;
        if (F7_h%2!=0) F7_h--; // only even numbers

		uint32_t F7_b = F7_bmax;
		//uint32_t F7_b = F7_bmax/5*4;
		
		dc1394_get_color_coding_from_video_mode(camera,video_mode, &coding);
		if (coding!=color_coding) {
			std::cerr << "problems getting color coding" << std::endl;
			return false;
		}

		if ( dc1394_video_set_mode(camera, video_mode) !=DC1394_SUCCESS) {
            std::cerr << "error setting FORMAT7" << std::endl;
			return false;
        }

		if ( dc1394_format7_set_roi(camera, video_mode, coding, F7_b,F7_x, F7_y, F7_w, F7_h) !=DC1394_SUCCESS) {
            std::cerr << "error setting up FORMAT7" << std::endl;
			return false;
        }

	} else { 
		// get video modes
		if (dc1394_video_get_supported_modes(camera,&video_modes)!=DC1394_SUCCESS) {
			fprintf(stderr,"can't get video modes\n");
			//if (camera!=NULL) dc1394_camera_free(camera);
			return false;
		}
		
		// get video mode
		for (int i=video_modes.num-1;i>=0;i--) {
			if (!dc1394_is_video_mode_scalable(video_modes.modes[i])) {
				dc1394_get_color_coding_from_video_mode(camera,video_modes.modes[i], &coding);
				if ((cfg->cam_width==width_table[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN]) && (cfg->cam_height==height_table[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN])) {
					if (coding==color_coding) {
						video_mode=video_modes.modes[i];
						break;
					}
				}
			}
		}

		if (coding != color_coding) return false;
		dc1394_video_set_mode(camera, video_mode);
		
		// get framerate
		dc1394_video_get_supported_framerates(camera,video_mode,&framerates);
		framerate=framerates.framerates[framerates.num-1];
		for (unsigned int i=0;i<framerates.num;i++) {
			if (cfg->cam_fps == framerate_table[framerates.framerates[i]-DC1394_FRAMERATE_MIN]) {
				framerate=framerates.framerates[i];
				break;
			}
		}

		// set framerate
		dc1394_video_set_framerate(camera, framerate);
		dc1394_video_get_framerate(camera, &framerate);
		cfg->cam_fps = framerate_table[framerate-DC1394_FRAMERATE_MIN];
	} //standard format


	if (dc1394_capture_setup(camera,4, DC1394_CAPTURE_FLAGS_DEFAULT)!=DC1394_SUCCESS) {
		fprintf( stderr,"unable to setup camera\n");
		return false;
	}

	unsigned int w, h;
	dc1394_get_image_size_from_video_mode(camera, video_mode, &w, &h);

    cfg->cam_width  = cfg->frame_width  = w;
	cfg->cam_height = cfg->frame_height = h;

	cam_buffer = new unsigned char[cfg->cam_width*cfg->cam_height*cfg->buf_format];

    applyCameraSettings();
	return true;
}

unsigned char* DC1394Camera::getFrame()
{
	dc1394video_frame_t *frame = NULL;
	dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &frame);
	if (frame==NULL) {
		if (lost_frames>timeout) { // give up after 5 (at init) or 2 (at runtime) seconds
			running = false;
		} lost_frames++;
		usleep(1000);
		return NULL;
	}

	dc1394_capture_enqueue(camera, frame);
	lost_frames=0;
	timeout = 2000;

	if (cfg->color) {
		if (coding==DC1394_COLOR_CODING_RGB8) return frame->image;
		else if (coding==DC1394_COLOR_CODING_YUV411) {
            uyvy2rgb(cfg->cam_width, cfg->cam_height, (unsigned char*)frame->image, cam_buffer);
            return cam_buffer;
		} else { return NULL; }
	} else {

		if (coding==DC1394_COLOR_CODING_MONO8) return frame->image;
		if (coding==DC1394_COLOR_CODING_MONO16) {
			unsigned char *src = (unsigned char*)frame->image;
			unsigned char *dest = cam_buffer;
			int size = cfg->cam_height*cfg->cam_width*2;
			for(int i=0;i<size;i+=2) {
				//unsigned short value = (src[ i] << 8 | src[ i+1]);
				//*dest++ = (unsigned char)(value/256);
				
				*dest++ = (unsigned char)((src[ i] << 8 | src[ i+1])/256);
			} return cam_buffer;
		}
		else if (coding==DC1394_COLOR_CODING_YUV411) {
			unsigned char *src = (unsigned char*)frame->image;
			unsigned char *dest = cam_buffer;
            int size = cfg->cam_height*cfg->cam_width/2;
			for(int i=size;i>0;i--) {
				src++;
				*dest++ = *src++;
				*dest++ = *src++;
			} return cam_buffer;
		} else if (coding==DC1394_COLOR_CODING_YUV422) {
		    uyvy2gray(cfg->cam_width, cfg->cam_height, (unsigned char*)frame->image, cam_buffer);
            return cam_buffer;
		} else { return NULL; }
	}
}

bool DC1394Camera::startCamera()
{

	if (dc1394_video_set_transmission(camera, DC1394_ON) !=DC1394_SUCCESS) {
		fprintf( stderr, "unable to start camera iso transmission\n");
		//if (camera!=NULL) dc1394_camera_free(camera);
	}

	dc1394switch_t status = DC1394_OFF;

	int i = 0;
	while( status == DC1394_OFF && i++ < 5 ) {
		usleep(50000);
		if (dc1394_video_get_transmission(camera, &status)!=DC1394_SUCCESS) {
			fprintf(stderr, "unable to get transmision status\n");
			return false;
		}
	}

	if( i == 5 ) {
		fprintf(stderr,"camera doesn't seem to want to turn on\n");
		return false;
	}

	running = true;
	return true;
}

bool DC1394Camera::stopCamera()
{
	if(!running) return false;

	if (dc1394_video_set_transmission(camera,DC1394_OFF)!=DC1394_SUCCESS) {
		//printf("couldn't stop the camera?\n");
		//return false;
	}

	if (dc1394_capture_stop(camera)!=DC1394_SUCCESS) {
		//printf("couldn't stop the camera?\n");
		//return false;
	}


	running=false;
	return true;
}

bool DC1394Camera::stillRunning() {
	return running;
}

bool DC1394Camera::resetCamera()
{
  return (stopCamera() && startCamera());
}

bool DC1394Camera::closeCamera()
{
	updateSettings();
	if (camera!=NULL) dc1394_camera_free(camera);
	if (d!=NULL) dc1394_free(d);
	return true;
}

bool DC1394Camera::hasCameraSetting(int mode) {

	dc1394feature_info_t info;
	info.id = (dc1394feature_t)-1;
	switch (mode) {
		case BRIGHTNESS: info.id = DC1394_FEATURE_BRIGHTNESS; break;
		case GAIN: info.id = DC1394_FEATURE_GAIN; break;
		case AUTO_GAIN: return hasCameraSettingAuto(GAIN);
		case SHUTTER: info.id = DC1394_FEATURE_SHUTTER; break;
		case EXPOSURE: info.id = DC1394_FEATURE_EXPOSURE; break;
		case AUTO_EXPOSURE: return hasCameraSettingAuto(EXPOSURE);
		case SHARPNESS: info.id = DC1394_FEATURE_SHARPNESS; break;
		case FOCUS: info.id = DC1394_FEATURE_FOCUS; break;
		case AUTO_FOCUS: return hasCameraSettingAuto(FOCUS);
		case GAMMA: info.id = DC1394_FEATURE_GAMMA; break;
		case WHITE: info.id = DC1394_FEATURE_WHITE_BALANCE; break;
		case AUTO_WHITE: return hasCameraSettingAuto(WHITE);
		case SATURATION: info.id = DC1394_FEATURE_SATURATION; break;
		case COLOR_HUE: info.id = DC1394_FEATURE_HUE; break;
		case AUTO_HUE: return hasCameraSettingAuto(COLOR_HUE);
	} if (info.id<0) return false;
	
	if ( dc1394_feature_get(camera, &info) != DC1394_SUCCESS) return false;

	return info.available;
}

bool DC1394Camera::setCameraSettingAuto(int mode, bool flag) {

	dc1394feature_t feature = (dc1394feature_t)-1;
	switch (mode) {
		case GAIN: feature = DC1394_FEATURE_GAIN; break;
		case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
		case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
	} if (feature<0) return false;

	dc1394feature_mode_t feature_mode = DC1394_FEATURE_MODE_AUTO;
	if (flag==false) feature_mode = DC1394_FEATURE_MODE_MANUAL;
	
	dc1394_feature_set_power(camera, feature, DC1394_ON);
	if ( dc1394_feature_set_mode(camera, feature, feature_mode) != DC1394_SUCCESS) return false;

	return true;
}

bool DC1394Camera::getCameraSettingAuto(int mode) {

    dc1394feature_t feature = (dc1394feature_t)-1;
    switch (mode) {
        case GAIN: feature = DC1394_FEATURE_GAIN; break;
        case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
        case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
    } if (feature<0) return false;

	dc1394feature_mode_t fmode;
	if ( dc1394_feature_get_mode(camera, feature, &fmode) != DC1394_SUCCESS) return false;
	
	if (fmode==DC1394_FEATURE_MODE_AUTO) return true;
	return false;
}

bool DC1394Camera::hasCameraSettingAuto(int mode) {
	dc1394feature_info_t info;
	info.id = (dc1394feature_t)-1;
	switch (mode) {
		case GAIN: info.id = DC1394_FEATURE_GAIN; break;
		case EXPOSURE: info.id = DC1394_FEATURE_EXPOSURE; break;
		case FOCUS: info.id = DC1394_FEATURE_FOCUS; break;
		case WHITE: info.id = DC1394_FEATURE_WHITE_BALANCE; break;
		case COLOR_HUE: info.id = DC1394_FEATURE_HUE; break;
	} if (info.id<0) return false;
	
	if ( dc1394_feature_get(camera, &info) != DC1394_SUCCESS) return false;
	if (!info.available) return false;

	for (unsigned int i=0;i<info.modes.num;i++) {
		if (info.modes.modes[i] == DC1394_FEATURE_MODE_AUTO) return true;
	}
	
	return false;
}

bool DC1394Camera::setCameraSetting(int mode, int setting) {

	int current_setting = getCameraSetting(mode);
	if (setting==current_setting) return true;

	dc1394feature_t feature = (dc1394feature_t)-1;
	switch (mode) {
		case BRIGHTNESS: feature = DC1394_FEATURE_BRIGHTNESS; break;
		case GAIN: feature = DC1394_FEATURE_GAIN; break;
		case AUTO_GAIN: return setCameraSettingAuto(GAIN,setting);
		case SHUTTER: feature = DC1394_FEATURE_SHUTTER; break;
		case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
		case AUTO_EXPOSURE: return setCameraSettingAuto(EXPOSURE,setting);
		case SHARPNESS: feature = DC1394_FEATURE_SHARPNESS;  break;
		case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case AUTO_FOCUS: return setCameraSettingAuto(FOCUS,setting);
		case GAMMA: feature = DC1394_FEATURE_GAMMA; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case AUTO_WHITE: return setCameraSettingAuto(WHITE,setting);
		case SATURATION: feature = DC1394_FEATURE_SATURATION; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
		case AUTO_HUE: return setCameraSettingAuto(COLOR_HUE,setting);
	} if (feature<0) return false;

	uint32_t value = (uint32_t)setting;
	dc1394_feature_set_power(camera, feature, DC1394_ON);
	dc1394_feature_set_mode(camera, feature, DC1394_FEATURE_MODE_MANUAL);
	if (dc1394_feature_set_value(camera, feature, value)!= DC1394_SUCCESS) return false;

	return true;
}

int DC1394Camera::getCameraSetting(int mode) {
	dc1394feature_t feature = (dc1394feature_t)-1;
	switch (mode) {
		case BRIGHTNESS: feature = DC1394_FEATURE_BRIGHTNESS; break;
		case GAIN: feature = DC1394_FEATURE_GAIN; break;
		case AUTO_GAIN: return getCameraSettingAuto(GAIN);
		case SHUTTER: feature = DC1394_FEATURE_SHUTTER; break;
		case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
		case AUTO_EXPOSURE: return getCameraSettingAuto(EXPOSURE);
		case SHARPNESS: feature = DC1394_FEATURE_SHARPNESS; break;
		case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case AUTO_FOCUS: return getCameraSettingAuto(FOCUS);
		case GAMMA: feature = DC1394_FEATURE_GAMMA; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case AUTO_WHITE: return getCameraSettingAuto(WHITE);
		case SATURATION: feature = DC1394_FEATURE_SATURATION; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
		case AUTO_HUE: return getCameraSettingAuto(COLOR_HUE);
	} if (feature<0) return 0;

	uint32_t value = 0;
	if (dc1394_feature_get_value(camera,  feature, &value)!= DC1394_SUCCESS) return 0;
	return (int)value;
}

int DC1394Camera::getMaxCameraSetting(int mode) {
	dc1394feature_t feature = (dc1394feature_t)-1;
	switch (mode) {
		case BRIGHTNESS: feature = DC1394_FEATURE_BRIGHTNESS; break;
		case GAIN: feature = DC1394_FEATURE_GAIN; break;
		case AUTO_GAIN: return 1;
		case SHUTTER: feature = DC1394_FEATURE_SHUTTER; break;
		case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
		case AUTO_EXPOSURE: return 1;
		case SHARPNESS: feature = DC1394_FEATURE_SHARPNESS; break;
		case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case AUTO_FOCUS: return 1;
		case GAMMA: feature = DC1394_FEATURE_GAMMA; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case AUTO_WHITE: return 1;
		case SATURATION: feature = DC1394_FEATURE_SATURATION; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
		case AUTO_HUE: return 1;
	} if (feature<0) return 0;

	uint32_t min;
	uint32_t max;
	if (dc1394_feature_get_boundaries(camera, feature, &min, &max)!= DC1394_SUCCESS) return 0;
	return (int)max;
}

int DC1394Camera::getMinCameraSetting(int mode) {
	dc1394feature_t feature = (dc1394feature_t)-1;
	switch (mode) {
		case BRIGHTNESS: feature = DC1394_FEATURE_BRIGHTNESS; break;
		case GAIN: feature = DC1394_FEATURE_GAIN; break;
		case AUTO_GAIN: return 0;
		case SHUTTER: feature = DC1394_FEATURE_SHUTTER; break;
		case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
		case AUTO_EXPOSURE: return 0;
		case SHARPNESS: feature = DC1394_FEATURE_SHARPNESS; break;
		case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case AUTO_FOCUS: return 0;
		case GAMMA: feature = DC1394_FEATURE_GAMMA; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case AUTO_WHITE: return 0;
		case SATURATION: feature = DC1394_FEATURE_SATURATION; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
		case AUTO_HUE: return 0;
	} if (feature<0) return 0;

	uint32_t min;
	uint32_t max;
	if (dc1394_feature_get_boundaries(camera, feature, &min, &max)!= DC1394_SUCCESS) return 0;
	return (int)min;
}

int DC1394Camera::getCameraSettingStep(int mode) {
	dc1394feature_t feature = (dc1394feature_t)-1;
	switch (mode) {
		case BRIGHTNESS: feature = DC1394_FEATURE_BRIGHTNESS; break;
		case GAIN: feature = DC1394_FEATURE_GAIN; break;
		case AUTO_GAIN: return 1;
		case SHUTTER: feature = DC1394_FEATURE_SHUTTER; break;
		case EXPOSURE: feature = DC1394_FEATURE_EXPOSURE; break;
		case AUTO_EXPOSURE: return 1;
		case SHARPNESS: feature = DC1394_FEATURE_SHARPNESS; break;
		case FOCUS: feature = DC1394_FEATURE_FOCUS; break;
		case AUTO_FOCUS: return 1;
		case GAMMA: feature = DC1394_FEATURE_GAMMA; break;
		case WHITE: feature = DC1394_FEATURE_WHITE_BALANCE; break;
		case AUTO_WHITE: return 1;
		case SATURATION: feature = DC1394_FEATURE_SATURATION; break;
		case COLOR_HUE: feature = DC1394_FEATURE_HUE; break;
		case AUTO_HUE: return 1;
	} if (feature<0) return 0;

	return 1;
}

bool DC1394Camera::setDefaultCameraSetting(int mode) {

    return true;
}

int DC1394Camera::getDefaultCameraSetting(int mode) {

    return INT_MIN;
}
