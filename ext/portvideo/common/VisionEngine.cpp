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


#include "VisionEngine.h"

void gosleep(int ms=1) {
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
                gosleep(1);
            } else {
                if (!engine->camera_->stillRunning()) {
                    engine->running_=false;
                    engine->error_=true;
                } else gosleep(1);
            }
        } else gosleep(5);
    }
    return(0);
}

#ifndef NDEBUG
void VisionEngine::saveBuffer(unsigned char* buffer) {
    
    int zerosize = 16-(int)floor(log10((float)framenumber_));
    if (zerosize<0) zerosize = 0;
    char zero[255];
    zero[zerosize]=0;
    for (int i=0;i<(zerosize);i++) zero[i]=48;
    
    char fileName[256];
#ifdef WIN32
    sprintf(fileName,".\\recording\\%s%ld.pgm",zero,framenumber_);
#else
    sprintf(fileName,"./recording/%s%ld.pgm",zero,framenumber_);
#endif
    std::cout << fileName << std::endl;
    FILE*  imagefile=fopen(fileName, "w");
    fprintf(imagefile,"P5\n%u %u 255\n", width_, height_);
    fwrite((const char *)buffer, 1,  width_*height_, imagefile);
    fclose(imagefile);
}
#endif

// the principal program sequence
void VisionEngine::start() {
    
    if (interface_) displayBuffer_ = interface_->openDisplay(this);
    
    if(camera_==NULL ) {
        if (interface_) interface_->displayError("No camera found!");
        else printf("No camera found!\n");
        return;
    }
    
    if( camera_->startCamera() ) {
        
        startThread();
        
        // add the help message from all FrameProcessors
        initFrameProcessors();
        for (frame = processorList.begin(); frame!=processorList.end(); frame++) {
            std::vector<std::string> processor_text = (*frame)->getOptions();
            if (processor_text.size()>0) help_text.push_back("");
            for(std::vector<std::string>::iterator processor_line = processor_text.begin(); processor_line!=processor_text.end(); processor_line++) {
                help_text.push_back(*processor_line);
            }
        }
        
        //print the help message
        std::cout << std::endl;
        for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++) {
            std::cout << *help_line << std::endl;
        } std::cout << std::endl;
        
        mainLoop();
        endLoop();
        stopThread();
        
    } else {
        if (interface_) interface_->displayError("Could not start camera!");
        else printf("Could not start camera!");
    }
    
    teardownCamera();
    freeBuffers();
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

void VisionEngine::pause(bool pause) {
    pause_ = pause;
}

void VisionEngine::mainLoop()
{
    unsigned char* cameraReadBuffer = NULL;
    
    while(running_) {
        
        // do nothing if paused
        if (pause_){
            if (interface_) interface_->processEvents();
            gosleep(1);
            continue;
        }
        
        //long start_time = currentMicroSeconds();
        cameraReadBuffer = ringBuffer->getNextBufferToRead();
        // loop until we get access to a frame
        while (cameraReadBuffer==NULL) {
            if(!running_) return;
            if (interface_) interface_->processEvents();
            gosleep(1);
            cameraReadBuffer = ringBuffer->getNextBufferToRead();
            //if (cameraReadBuffer!=NULL) break;
        }
        //long camera_time = currentMicroSeconds()-start_time;

        // do the actual image processing job
        for (frame = processorList.begin(); frame!=processorList.end(); frame++)
            (*frame)->process(cameraReadBuffer,destBuffer_,displayBuffer_);
        //long processing_time = currentMicroSeconds()-start_time;
        
#ifndef NDEBUG
        if (recording_) {
            if (interface_!=NULL) if (interface_->getDisplayMode()!=interface_->SOURCE_DISPLAY)
                memcpy(sourceBuffer_,cameraReadBuffer,ringBuffer->size());
            saveBuffer(sourceBuffer_);
        }
#endif
        if (interface_!=NULL) if (interface_->getDisplayMode()==interface_->SOURCE_DISPLAY)
            memcpy(sourceBuffer_,cameraReadBuffer,ringBuffer->size());
        ringBuffer->readFinished();
        
        if (interface_ && running_ ) {
            camera_->showInterface(interface_);
            interface_->updateDisplay();
        }
        
        //long total_time = currentMicroSeconds()-start_time;
        //frameStatistics(camera_time,processing_time,total_time);

    }
}

void VisionEngine::endLoop() {
    // finish all FrameProcessors
    for (frame = processorList.begin(); frame!=processorList.end(); frame++)
        (*frame)->finish();
    
    if(error_) {
        if (interface_) interface_->displayError("Camera disconnected!");
        else printf("Camera disconnected!\n");
    }
}

void VisionEngine::stop() {
    std::cout << "terminating " << app_name_ << " ... " << std::endl;
    running_ = false;
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
    //printf("%d\n",key);
    if( key == KEY_O ){
        display_lock_ = camera_->showSettingsDialog(display_lock_);
    }
#ifndef NDEBUG
    else if( key == KEY_M ){
        if (recording_) {
            recording_ = false;
        } else {
            struct stat info;
#ifdef WIN32
            if (stat(".\\recording",&info)!=0) {
                std::string dir(".\\recording");
                LPSECURITY_ATTRIBUTES attr = NULL;
                CreateDirectory(dir.c_str(),attr);
            }
#else
            if (stat("./recording",&info)!=0) mkdir("./recording",0777);
#endif
            recording_ = true;
        }
    } else if( key == KEY_M ){
        struct stat info;
#ifdef WIN32
        if (stat(".\\recording",&info)!=0) {
            std::string dir(".\\recording");
            LPSECURITY_ATTRIBUTES attr = NULL;
            CreateDirectory(dir.c_str(),attr);
        }
#else
        if (stat("./recording",&info)!=0) mkdir("./recording",0777);
#endif
        saveBuffer(sourceBuffer_);
    }
#endif
    
    camera_->control(key);
    for (frame = processorList.begin(); frame!=processorList.end(); frame++)
        display_lock_ = (*frame)->toggleFlag(key,display_lock_);
    
}

void VisionEngine::allocateBuffers()
{
    bytesPerSourcePixel_ = 1;
    bytesPerDestPixel_ = 1;
    
    if ((camera_!=NULL) && (camera_->getColor())) {
        bytesPerSourcePixel_ = 3;
        bytesPerDestPixel_ = 3;
    }
    
    sourceBuffer_  = new unsigned char[3*width_*height_];
    destBuffer_    = new unsigned char[3*width_*height_];
    ringBuffer = new RingBuffer(width_*height_*bytesPerSourcePixel_);
}

void VisionEngine::freeBuffers()
{
    delete [] sourceBuffer_;
    delete [] destBuffer_;
    delete ringBuffer;
}

void VisionEngine::addFrameProcessor(FrameProcessor *fp) {
    
    processorList.push_back(fp);
    if (interface_!=NULL) fp->addUserInterface(interface_);
}


void VisionEngine::removeFrameProcessor(FrameProcessor *fp) {
    frame = std::find( processorList.begin(), processorList.end(), fp );
    if( frame != processorList.end() ) {
        processorList.erase( frame );
    }
}

void VisionEngine::initFrameProcessors() {
    for (frame = processorList.begin(); frame!=processorList.end(); ) {
        bool success = (*frame)->init(width_ , height_, bytesPerSourcePixel_, bytesPerDestPixel_);
        if(!success) {
            processorList.erase( frame );
            printf("removed frame processor\n");
        } else frame++;
    }
}

void VisionEngine::setupCamera(CameraConfig *config) {
    
    camera_ = CameraTool::getCamera(config);
    if (camera_ == NULL) {
        allocateBuffers();
        return;
    }
    
    if(camera_->initCamera()) {
        width_ = camera_->getWidth();
        height_ = camera_->getHeight();
        fps_ = camera_->getFps();
        
        printf("camera: %s\n",camera_->getName());
        if (fps_>0) printf("format: %dx%d, %dfps\n",width_,height_,fps_);
        else printf("format: %dx%d\n",width_,height_);
    } else {
        printf("could not initialize camera\n");
        camera_->closeCamera();
        delete camera_;
        camera_ = NULL;
    }
    
    allocateBuffers();
}

void VisionEngine::teardownCamera()
{
    if (camera_!=NULL) {
        camera_->stopCamera();
        camera_->closeCamera();
        delete camera_;
        camera_ = NULL;
    }
}

void VisionEngine::setInterface(UserInterface *uiface) {
    if (uiface!=NULL) {
        interface_ = uiface;
        bool color = false;
        if (camera_) color = camera_->getColor();
        interface_->setBuffers(sourceBuffer_,destBuffer_,width_,height_,color);
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
{
    app_config_ = config;
    camera_config_ = CameraTool::readSettings(app_config_->camera_config);
    setupCamera(camera_config_);
    displayBuffer_ = NULL;
    
    lastTime_ = currentSeconds();
    cameraTime_ = processingTime_ = interfaceTime_ = totalTime_ = 0.0f;
    
    app_name_ = std::string(name);
    
    help_text.push_back("display:");
    help_text.push_back("   n - no image");
    help_text.push_back("   s - source image");
    help_text.push_back("   t - target image");
    help_text.push_back("   h - toggle help text");
    help_text.push_back("  F1 - toggle fullscreen");
    help_text.push_back("");
    help_text.push_back("commands:");
    help_text.push_back("   ESC - quit " + app_name_);
    help_text.push_back("   v - verbose output");
    help_text.push_back("   o - camera options");
    help_text.push_back("   p - pause processing");
#ifndef NDEBUG
    help_text.push_back("debug options:");
    help_text.push_back("   b - save buffer as PGM");
    help_text.push_back("   m - save buffer sequence");
#endif
}



