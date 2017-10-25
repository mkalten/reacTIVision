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

#include "MultiCamera.h"

std::vector<CameraConfig> MultiCamera::getCameraConfigs(std::vector<CameraConfig> child_cams) {
    // TODO: THIS NEEDS REWORK.
    // New approach could be to just list all available cams together as one additional entry.

    //there should be only equal, desired cams connected
    std::vector<CameraConfig> cfg_list;

    if (child_cams.size() <= 1)
        return cfg_list;

    std::vector<std::vector<CameraConfig>>::iterator iter_grp;
    std::vector<CameraConfig>::iterator iter;
    std::vector<std::vector<CameraConfig>> grp_cams;

    //Group compatible cams
    for (iter = child_cams.begin(); iter != child_cams.end(); iter++) {
        CameraConfig cam_cfg = *iter;

        bool grp_found = false;
        for (iter_grp = grp_cams.begin(); iter_grp != grp_cams.end(); iter_grp++) {
            CameraConfig first_cam_in_grp = *(*iter_grp).begin();

            if (
                first_cam_in_grp.cam_width == cam_cfg.cam_width &&
                first_cam_in_grp.cam_height == cam_cfg.cam_height &&
                first_cam_in_grp.cam_fps == cam_cfg.cam_fps &&
                first_cam_in_grp.cam_format == cam_cfg.cam_format &&
                first_cam_in_grp.driver == cam_cfg.driver
            ) {
                (*iter_grp).push_back(cam_cfg);
                grp_found = true;
            }
        }

        if (!grp_found) {
            std::vector<CameraConfig> grp;
            grp.push_back(cam_cfg);
            grp_cams.push_back(grp);
        }
    }

    //Create some example multicam configurations
    for (iter_grp = grp_cams.begin(); iter_grp != grp_cams.end(); iter_grp++) {
        int cams_cnt = (*iter_grp).size();
        CameraConfig first_child_in_grp = *(*iter_grp).begin();

        CameraConfig cfg;
        CameraTool::initCameraConfig(&cfg);
        cfg.driver = DRIVER_MUTLICAM;

        // create name
        sprintf(cfg.name, "MultiCam (");
        // list all used camera device ids
        std::vector<CameraConfig>::iterator iter_grp_cam;
        for (iter_grp_cam = (*iter_grp).begin(); iter_grp_cam != (*iter_grp).end(); iter_grp_cam++) {
            sprintf(
                cfg.name + strlen(cfg.name),
                "%i",
                (* iter_grp_cam).device
            );
            if (iter_grp_cam != (*iter_grp).end()) {
                sprintf(cfg.name + strlen(cfg.name), ", ");
            }
        }
        sprintf(cfg.name + strlen(cfg.name), ")");

        cfg.cam_width = first_child_in_grp.cam_width * cams_cnt;
        cfg.cam_height = first_child_in_grp.cam_height;
        cfg.cam_fps = first_child_in_grp.cam_fps;
        cfg.cam_format = first_child_in_grp.cam_format;

        // TODO: creeate child configuration with correct x offset
        cfg.childs.insert(
            cfg.childs.end(),
            (*iter_grp).begin(),
            (*iter_grp).end()
        );
        cfg_list.push_back(cfg);
    }

    return cfg_list;
}

CameraEngine* MultiCamera::getCamera(CameraConfig* cam_cfg) {
	MultiCamera *cam = new MultiCamera(cam_cfg);
    #ifdef DEBUG
        if (cam) {
            std::cout << "creating  MultiCamera succeed" << std::endl;
        } else {
            std::cout << "creating  MultiCamera failed" << std::endl;;
        }
    #endif
	return cam;
}

MultiCamera::MultiCamera(CameraConfig *cam_cfg) : CameraEngine(cam_cfg) {
    std::vector<CameraConfig>::iterator iter;
    for (iter = cam_cfg->childs.begin(); iter != cam_cfg->childs.end(); iter++) {
        if (iter->color) {
            iter->buf_format = FORMAT_RGB;
        } else {
            iter->buf_format = FORMAT_GRAY;
        }
    }

    // show MultiCamera and child count as name
    int cams_cnt = (cam_cfg->childs).size();
    sprintf(cfg->name, "MultiCamera: %i childs", cams_cnt);

	frm_buffer = NULL;
	cam_buffer = NULL;

    cam_cfg->driver = DRIVER_MUTLICAM;
}

MultiCamera::~MultiCamera() {

	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		delete *iter;
    }

	if (cam_buffer) {
		delete[] cam_buffer;
		cam_buffer = NULL;
	}
}

void MultiCamera::printInfo() {
	printf("Multicam: %i childs\n", (int)cameras_.size());

	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		(*iter)->printInfo();
		printf("\n");
	}
}

bool MultiCamera::initCamera() {

    #ifdef DEBUG
        printf("MultiCamera::initCamera\n");
    #endif

    if (cfg == NULL || cfg->childs.size() <= 0) {
        std::cout << "no child cameras spezified in multicam.xml" << std::endl;
        return false;
    }

    CameraConfig first_cfg = *cfg->childs.begin();

    //  check for cascaded multicam drivers
	std::vector<CameraConfig>::iterator iter;
    for (iter = cfg->childs.begin(); iter != cfg->childs.end(); iter++) {
        CameraConfig* child_cfg = &(*iter);
        if (child_cfg->driver == DRIVER_MUTLICAM) {
            std::cout << "cascading multicam driver not supported" << std::endl;
            return false;
        }
    }

	for (iter = cfg->childs.begin(); iter != cfg->childs.end(); iter++) {
        CameraConfig* child_cfg = &(*iter);
		CameraEngine* cam = CameraTool::getCamera(child_cfg, false);
		if (cam == NULL) {
			std::cout << "error creating child cam" << child_cfg->device << std::endl;
			return false;
		}

		if (!cam->initCamera()) {
			delete cam;
            #ifdef DEBUG
                std::cout << "error init child camera" << child_cfg->device << std::endl;
            #endif
			return false;
		}

        cameras_.push_back(cam);
	}

	cfg->frame = false;
	setupFrame();

	if (!cam_buffer) {
        // define canvas buffer size
        const int buffer_size = cfg->frame_height * cfg->frame_width * cfg->buf_format;
        cam_buffer = new unsigned char[buffer_size];
        // clear cavas buffer
        memset(cam_buffer, 0, buffer_size);
    }

    #ifdef DEBUG
        printf("MultiCamera::initCamera done\n\n");
    #endif

	return true;
}

bool MultiCamera::closeCamera() {
	bool result = true;

	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		if ((*iter) == NULL) {
            continue;
        }

		result &= (*iter)->closeCamera();
	}

	if (cam_buffer) {
		delete[] cam_buffer;
		cam_buffer = NULL;
	}

	return result;
}

bool MultiCamera::startCamera() {
	bool result = true;

	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		result &= (*iter)->startCamera();
    }

	if (!result) {
		stopCamera();
    }

	running = result;

    #ifdef DEBUG
        std::cout << "MultiCamera::startCamera '" <<
        running << "'" << std::endl;
    #endif

	return result;
}

bool MultiCamera::stopCamera() {
	bool result = true;

	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		result &= (*iter)->stopCamera();
    }

	running = !result;

	return result;
}

bool MultiCamera::resetCamera() {
	bool result = true;

	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		result &= (*iter)->resetCamera();
    }

	return result;
}

unsigned char* MultiCamera::getFrame() {
    // #ifdef DEBUG
    //     std::cout << "MultiCamera::getFrame" << std::endl;
    // #endif

    // cam_buffer points to a buffer the size of the main image frame.
    // cfg->frame_height * cfg->frame_width * cfg->buf_format
	// unsigned char* childcam_buffer_start = cam_buffer;

    // clear cavas buffer
    // const int buffer_size = cfg->frame_height * cfg->frame_width * cfg->buf_format;
    // memset(cam_buffer, 10, buffer_size);

    // int cam_buffer_line_size = cfg->frame_width * cfg->buf_format;

    const int canvas_width = cfg->frame_width;
    const int canvas_height = cfg->frame_height;
    const int canvas_format = cfg->buf_format;

    // clear cavas buffer
    // const int canvas_buffer_size = canvas_width * canvas_height * canvas_format;
    // memset(cam_buffer, 10, canvas_buffer_size);


    // for every child cam
    std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
        // get current child cam
		CameraEngine* cam = *iter;

        // get child frame
        unsigned char* child_frame = cam->getFrame();
		if (child_frame == NULL) {
			this->running = false;
			return NULL;
		}

        const int child_width = cam->getWidth();
        const int child_height = cam->getHeight();
        const int child_format = cam->getFormat();


        int child_x_start = cam->getFrameX();
        // check and restrict left bound of canvas
        if (child_x_start < 0) {
            child_x_start = 0;
        }

        // check and restrict top bound of canvas
        int child_y_start = cam->getFrameY();
        if (child_y_start < 0) {
            child_y_start = 0;
        }

        // check and restrict right bound of canvas
        // int child_x_end = child_x_start + child_width;
        // if (child_x_end > canvas_width) {
        //     // image of child cam overflows right border of canvas
        //     // so we restrict to canvas area
        //     child_x_end = canvas_width;
        // }
        // int copy_size = child_x_end * child_format;
        int child_x_end = child_x_start + child_width;
        int copy_size = child_width * child_format;
        if (child_x_end > canvas_width) {
            // image of child cam overflows right border of canvas
            // so we restrict to canvas area
            child_x_end = canvas_width;
            // recalculate child_width
            int child_width_cropped = child_x_end - child_x_start;
            copy_size = child_width_cropped * child_format;
        }

        // check and restrict bottom bound of canvas
        int child_y_end = child_y_start + child_height;
        int child_height_fenced = child_height;
        if (child_y_end > canvas_height) {
            child_y_end = canvas_height;
            // recalculate child_height_fenced
            child_height_fenced = child_y_end - child_y_start;
        }



        // for every line of the child height
		for (
            // int child_line_index = child_y_start;
            int child_line_index = 0;
            // child_line_index < child_y_end;
            child_line_index < child_height_fenced;
            child_line_index++
        ) {
            // find start position on canvas
            unsigned char* buffer_write_start = (
                cam_buffer +
                (
                    (
                        (
                            (child_line_index + child_y_start)
                            * canvas_width
                        )
                        + child_x_start
                     )
                    * canvas_format
                )
            );

            // find start position in child
            // unsigned char* child_start = (
            //     child_frame +
            //     (
            //
            //         (
            //             ((child_y_start + child_line_index) * child_width )
            //             + child_x_start
            //         )
            //         * child_format
            //     )
            // );

            unsigned char* child_start = (
                child_frame +
                (
                    ((child_line_index) * child_width )
                    * child_format
                )
            );

            // copy line
			memcpy(buffer_write_start, child_start, copy_size);
            // if (child_line_index == child_height) {
            //     memset(buffer_write_start, 255, copy_size);
            // }

		}

	}

    // #ifdef DEBUG
    //     std::cout << "MultiCamera::getFrame done " << std::endl;
    // #endif

	return cam_buffer;
}

int MultiCamera::getCameraSettingStep(int mode) {
    return cameras_[0]->getCameraSettingStep(mode);
}

int MultiCamera::getCameraSetting(int mode) {
    return cameras_[0]->getCameraSetting(mode);
}

int MultiCamera::getMaxCameraSetting(int mode) {
    return cameras_[0]->getMaxCameraSetting(mode);
}

int MultiCamera::getMinCameraSetting(int mode) {
    return cameras_[0]->getMinCameraSetting(mode);
}

bool MultiCamera::getCameraSettingAuto(int mode) {
    return cameras_[0]->getCameraSettingAuto(mode);
}

int MultiCamera::getDefaultCameraSetting(int mode) {
    return cameras_[0]->getDefaultCameraSetting(mode);
}

bool MultiCamera::hasCameraSetting(int mode) {
    return cameras_[0]->hasCameraSetting(mode);
}

bool MultiCamera::hasCameraSettingAuto(int mode) {
    return cameras_[0]->hasCameraSettingAuto(mode);
}


bool MultiCamera::setCameraSettingAuto(int mode, bool flag) {
	bool result = true;
	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		result &= (*iter)->setCameraSettingAuto(mode, flag);
    }
	return result;
}

bool MultiCamera::setCameraSetting(int mode, int value) {
	bool result = true;
	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		result &= (*iter)->setCameraSetting(mode, value);
    }
	return result;
}

bool MultiCamera::setDefaultCameraSetting(int mode) {
	bool result = true;
	std::vector<CameraEngine*>::iterator iter;
	for (iter = cameras_.begin(); iter != cameras_.end(); iter++) {
		result &= (*iter)->setDefaultCameraSetting(mode);
    }
	return result;
}
