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


#include "VisionEngine.h"
#include "ConsoleInterface.h"

void pv_sleep(int ms) {
#ifndef WIN32
	usleep(ms*1000);
#else
	Sleep(ms);
#endif
}


// the thread function which constantly retrieves the latest frame
#ifndef WIN32
static void* getFrameFromCamera( void* obj )
#else
static DWORD WINAPI getFrameFromCamera( LPVOID obj )
#endif
{
    
    VisionEngine *engine = (VisionEngine *)obj;
    
    unsigned char *cameraBuffer = NULL;
    unsigned char *cameraWriteBuffer = NULL;
    
    while(engine->running_) {
        if(!engine->pause_) {
            //long start_time = VisionEngine::currentMicroSeconds();
            cameraBuffer = engine->camera_->getFrame();
            if (cameraBuffer!=NULL) {
                cameraWriteBuffer = engine->ringBuffer->getNextBufferToWrite();
                if (cameraWriteBuffer!=NULL) {
                    memcpy(cameraWriteBuffer,cameraBuffer,engine->ringBuffer->size());
                    engine->framenumber_++;
                    engine->ringBuffer->writeFinished();
                    //long driver_time = VisionEngine::currentMicroSeconds() - start_time;
                    //std::cout << "camera latency: " << driver_time/1000.0f << "ms" << std::endl;
                }
				pv_sleep();
            } else {
                if ((!engine->pause_) && (!engine->camera_->stillRunning())) {
                    engine->running_=false;
                    engine->error_=true;
                } else pv_sleep();
            }
        } else pv_sleep(5);
    }
    return(0);
}

#ifndef NDEBUG
void VisionEngine::saveBuffer(unsigned char* buffer, int bytes) {
	
	const char *file_ext = "pgm";
	if (bytes==3) file_ext = "ppm";
	
    struct stat info;
#ifdef WIN32
    if (stat(".\\recording",&info)!=0) {
        std::string dir(".\\recording");
        LPSECURITY_ATTRIBUTES attr = NULL;
        CreateDirectory(dir.c_str(),attr);
    }
#elif defined __APPLE__
    char path[1024];
    char full_path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
    CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);
    CFRelease( mainBundleURL);
    CFRelease( cfStringRef);
    sprintf(full_path,"%s/../recording",path);
    if (stat(full_path,&info)!=0) mkdir(full_path,0777);
#else
    if (stat("./recording",&info)!=0) mkdir("./recording",0777);
#endif
    
    int zerosize = 16-(int)floor(log10((float)framenumber_));
    if (zerosize<0) zerosize = 0;
    char zero[255];
    zero[zerosize]=0;
    for (int i=0;i<(zerosize);i++) zero[i]=48;
    
    char fileName[256];
#ifdef WIN32
    sprintf(fileName,".\\recording\\%s%ld.%s",zero,framenumber_,file_ext);
#elif defined __APPLE__
    sprintf(fileName,"%s/../recording/%s%ld.%s",path,zero,framenumber_,file_ext);
#else
    sprintf(fileName,"./recording/%s%ld.%s",zero,framenumber_,file_ext);
#endif

    FILE*  imagefile=fopen(fileName, "w");
    if (bytes==3) fprintf(imagefile,"P6\n%u %u 255\n", width_, height_);
	else fprintf(imagefile,"P5\n%u %u 255\n", width_, height_);
    fwrite((const char *)buffer, bytes,  width_*height_, imagefile);
    fclose(imagefile);
}
#endif

// the principal program sequence
void VisionEngine::start() {
    
	if (!interface_) interface_ = new ConsoleInterface(app_name_.c_str());
	
	if (!interface_->openDisplay(this)) {
		delete interface_;
		interface_ = new ConsoleInterface(app_name_.c_str());
		interface_->openDisplay(this);
	}
	
    if(camera_==NULL ) {
        interface_->displayError("No camera found!");
        return;
    }
    
    if( camera_->startCamera() ) {
        
        initFrameProcessors();
        startThread();
        mainLoop();
        stopThread();
        
    } else interface_->displayError("Could not start camera!");
    
    teardownCamera();
    freeBuffers();
}

void VisionEngine::stop() {
	std::cout << "terminating " << app_name_ << " ... " << std::endl;
	running_ = false;
	interface_->closeDisplay();
}

void VisionEngine::pause(bool pause) {
	
	pause_ = pause;
}

void VisionEngine::resetCamera(CameraConfig *cam_cfg) {

	//teardownCamera();
	freeBuffers();
	if (cam_cfg!=NULL) CameraTool::setCameraConfig(cam_cfg);
	setupCamera();
	
	if( camera_->startCamera() ) {
		interface_->closeDisplay();
		interface_->setBuffers(sourceBuffer_,destBuffer_,width_,height_,format_);
		interface_->openDisplay(this);
		for (frame = processorList.begin(); frame!=processorList.end();frame++)
			(*frame)->init(width_ , height_, format_, format_);
	} else interface_->displayError("Could not start camera!");
	
	pause_ = false;
}

void VisionEngine::startThread() {
    
    running_=true;
#ifndef WIN32
    pthread_create(&cameraThread , NULL, getFrameFromCamera, this);
#else
    DWORD threadId;
    cameraThread = CreateThread( 0, 0, getFrameFromCamera, this, 0, &threadId );
#endif
    
}

void VisionEngine::stopThread() {
    
#ifdef WIN32
    WaitForSingleObject(cameraThread,INFINITE);
    if( cameraThread ) CloseHandle( cameraThread );
#else
    if( cameraThread ) pthread_join(cameraThread,NULL);
#endif
    
}

void VisionEngine::mainLoop()
{
    unsigned char* cameraReadBuffer = NULL;
    
    while(running_) {
        
        // do nothing if paused
        if (pause_){
            interface_->processEvents();
            pv_sleep();
            continue;
        }
        
        //long start_time = currentMicroSeconds();
        cameraReadBuffer = ringBuffer->getNextBufferToRead();
        // loop until we get access to a frame
        while (cameraReadBuffer==NULL) {
            interface_->processEvents();
            if (!running_) {
                if(error_) interface_->displayError("Camera disconnected!");
                return;
            }
            pv_sleep();
            cameraReadBuffer = ringBuffer->getNextBufferToRead();
            //if (cameraReadBuffer!=NULL) break;
        }
        //long camera_time = currentMicroSeconds()-start_time;

        // do the actual image processing job
        for (frame = processorList.begin(); frame!=processorList.end(); frame++)
            (*frame)->process(cameraReadBuffer,destBuffer_);
        //long processing_time = currentMicroSeconds()-start_time;
  
        if (interface_->getDisplayMode()==SOURCE_DISPLAY)
            memcpy(sourceBuffer_,cameraReadBuffer,ringBuffer->size());
        ringBuffer->readFinished();
        
#ifndef NDEBUG
        if (recording_) {
			if (interface_->getDisplayMode()==SOURCE_DISPLAY)
				saveBuffer(sourceBuffer_,format_);
			else saveBuffer(destBuffer_,format_);
        }
#endif

        if (running_) {
			if (camera_) camera_->showInterface(interface_);
            interface_->updateDisplay();
        }
        //long total_time = currentMicroSeconds()-start_time;
        //frameStatistics(camera_time,processing_time,total_time);
    }
}

void VisionEngine::frameStatistics(long cameraTime, long processingTime, long totalTime) {
    
    frames_++;
    cameraTime_+=cameraTime;
    processingTime_+=(processingTime-cameraTime);
    interfaceTime_+=(totalTime-processingTime);
    totalTime_+=totalTime;
    
    long currentTime_ = currentSeconds();
    long diffTime = currentTime_ - lastTime_ ;
    
    if (diffTime >= 1) {
        current_fps_ = (int)floor( (frames_ / diffTime) + 0.5 );
        std::cout << current_fps_ << "fps ";
        
        std::cout << std::fixed << std::setprecision(2)
        << "c:" << (cameraTime_/frames_)/1000.0f
        << "ms p:" << (processingTime_/frames_)/1000.0f
        << "ms i:" << (interfaceTime_/frames_)/1000.0f
        << "ms t:" << (totalTime_/frames_)/1000.0f << "ms" << std::endl;
        
        cameraTime_ = processingTime_ = interfaceTime_ = totalTime_ = 0.0f;
        
        lastTime_ = currentTime_;
        frames_ = 0;
    }
}

void VisionEngine::event(int key)
{

    if( key == KEY_O ){
        display_lock_ = camera_->showSettingsDialog(display_lock_);
    }
#ifndef NDEBUG
    else if( key == KEY_M ){
        recording_ = !recording_;
    } else if( key == KEY_N ){
		if (interface_->getDisplayMode()==SOURCE_DISPLAY)
			saveBuffer(sourceBuffer_,format_);
		else saveBuffer(destBuffer_,format_);
    }
#endif
    
    //printf("%d\n",key);
    if (camera_) camera_->control(key);
    for (frame = processorList.begin(); frame!=processorList.end(); frame++)
        display_lock_ = (*frame)->toggleFlag(key,display_lock_);
    
}

void VisionEngine::allocateBuffers()
{
    sourceBuffer_  = new unsigned char[format_*width_*height_];
    destBuffer_    = new unsigned char[format_*width_*height_];
    ringBuffer = new RingBuffer(width_*height_*format_);
}

void VisionEngine::freeBuffers()
{
    delete [] sourceBuffer_;
    delete [] destBuffer_;
    delete ringBuffer;
}

void VisionEngine::addFrameProcessor(FrameProcessor *fp) {
    processorList.push_back(fp);
}


void VisionEngine::removeFrameProcessor(FrameProcessor *fp) {
    frame = std::find( processorList.begin(), processorList.end(), fp );
    if( frame != processorList.end() ) {
        processorList.erase( frame );
    }
}

void VisionEngine::initFrameProcessors() {
    
    std::vector<std::string> help_text;
    for (frame = processorList.begin(); frame!=processorList.end(); ) {
        bool success = (*frame)->init(width_ , height_, format_, format_);
        if(success) {
            
            std::vector<std::string> processor_text = (*frame)->getOptions();
            if (processor_text.size()>0) help_text.push_back("");
            for(std::vector<std::string>::iterator processor_line = processor_text.begin(); processor_line!=processor_text.end(); processor_line++) {
                help_text.push_back(*processor_line);
            }
            
    		(*frame)->addUserInterface(interface_);
            frame++;
        }  else processorList.erase( frame );
    }
    
    interface_->setHelpText(help_text);
}

void VisionEngine::setupCamera() {
    
    camera_ = CameraTool::getCamera(camera_config_);
	if (camera_ == NULL) {
        allocateBuffers();
        return;
    }
	
    if(camera_->initCamera()) {
        width_ = camera_->getWidth();
        height_ = camera_->getHeight();
        fps_ = camera_->getFps();
		format_ = camera_->getFormat();
		camera_->printInfo();
    } else {

		printf("could not initialize selected camera\n");
        camera_->closeCamera();
        delete camera_;
		camera_ = CameraTool::getDefaultCamera();

		if (camera_ == NULL) {
			allocateBuffers();
			return;
		} else if(camera_->initCamera()) {
			width_ = camera_->getWidth();
			height_ = camera_->getHeight();
			fps_ = camera_->getFps();
			format_ = camera_->getFormat();
			camera_->printInfo();
		} else {
			printf("could not initialize default camera\n");
			camera_->closeCamera();
			delete camera_;
			camera_ = NULL;
		}
    }
		
    allocateBuffers();
}

void VisionEngine::teardownCamera()
{
    if (camera_!=NULL) {
		pause_ = true;
        camera_->stopCamera();
        camera_->closeCamera();
        delete camera_;
        camera_ = NULL;
    }
}

void VisionEngine::setInterface(UserInterface *uiface) {
    if (uiface!=NULL) {
        interface_ = uiface;
        interface_->setBuffers(sourceBuffer_,destBuffer_,width_,height_,format_);
    }
}

VisionEngine::VisionEngine(const char* name, application_settings *config)
: error_( false )
, pause_( false )
, calibrate_( false )
, help_( false )
, display_lock_( false )
, current_fps_( 0 )
, camera_ (NULL)
, framenumber_( 0 )
#ifndef NDEBUG
, recording_( false )
#endif
, interface_ ( NULL )
, frames_( 0 )
, width_( WIDTH )
, height_( HEIGHT )
, format_( 1 )
{
    app_config_ = config;
    camera_config_ = CameraTool::readSettings(app_config_->camera_config);
    setupCamera();
	
    lastTime_ = currentSeconds();
    cameraTime_ = processingTime_ = interfaceTime_ = totalTime_ = 0.0f;
    
    app_name_ = std::string(name);
}



