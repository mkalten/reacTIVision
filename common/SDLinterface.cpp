/*  reacTIVision tangible interaction framework
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

#include "SDLinterface.h"

// the thread function which contantly retrieves the latest frame
int getFrameFromCamera(void *obj) {

		SDLinterface *engine = (SDLinterface *)obj;

		unsigned char *cameraBuffer = NULL;
		unsigned char *cameraWriteBuffer = NULL;

		while(engine->running_) {
			if(!engine->pause_) {
				//long start_time = SDLinterface::currentTime();
				cameraBuffer = engine->camera_->getFrame();
				if (cameraBuffer!=NULL) {
					cameraWriteBuffer = engine->ringBuffer->getNextBufferToWrite();
					if (cameraWriteBuffer!=NULL) {
						memcpy(cameraWriteBuffer,cameraBuffer,engine->ringBuffer->size());
						engine->framenumber_++;
						engine->ringBuffer->writeFinished();
						//long driver_time = SDLinterface::currentTime() - start_time;
						//std::cout << "camera latency: " << (driver_time/100)/10.0f << "ms" << std::endl;
					}
					SDL_Delay(1);
				} else {
					if (!engine->camera_->stillRunning()) {
						engine->running_=false;
						engine->error_=true;
					} else SDL_Delay(1);
				}
			} else SDL_Delay(5);
		}
		return(0);
}

#ifndef NDEBUG
void SDLinterface::saveBuffer(unsigned char* buffer) {

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

void SDLinterface::stop() {
	running_=false;
	error_=false;
}

// the principal program sequence
void SDLinterface::run() {

 	if( camera_==NULL ) {
		if( !setupWindow() ) return;
		allocateBuffers();
		showError("No camera found!");
		teardownWindow();
		freeBuffers();
		return;
	}

 	width_ = camera_->getWidth();
	height_ = camera_->getHeight();
	fps_ = camera_->getFps();

	if( !setupWindow() ) return;
	allocateBuffers();
	initFrameProcessors();

	bool success = camera_->startCamera();
 	if( success ) {
        
        running_=true;
        cameraThread = SDL_CreateThread(getFrameFromCamera,"cameraThread",this);

		// add the help message from all FrameProcessors
		for (frame = processorList.begin(); frame!=processorList.end(); frame++) {
                        std::vector<std::string> processor_text = (*frame)->getOptions();
			if (processor_text.size()>0) help_text.push_back("");
			for(std::vector<std::string>::iterator processor_line = processor_text.begin(); processor_line!=processor_text.end(); processor_line++) {
				help_text.push_back(*processor_line);
			}
		}
        
        if (!headless_) {
            
            if(fullscreen_) {
                SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
                SDL_ShowCursor(0);
            }
            
            if (displayMode_!=NO_DISPLAY) displayMessage("starting camera");
            //print the help message
            std::cout << std::endl;
            for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++) {
                std::cout << *help_line << std::endl;
            } std::cout << std::endl;
        }

		mainLoop();
		endLoop();

		SDL_WaitThread(cameraThread, NULL);
	} else {
		showError("Could not start camera!");
 	}

	teardownWindow();
	freeBuffers();
}

void SDLinterface::showError(const char* error)
{
    if (headless_) {
        printf("ERROR: %s\n",error);
        return;
    }

    SDL_SetWindowTitle(window_,app_name_.c_str());
	SDL_RenderClear(renderer_);

	SDL_Rect *image_rect = new SDL_Rect();
	image_rect->x = (width_-camera_rect.w)/2-47;
	image_rect->y = (height_-camera_rect.h)/2-39;
	image_rect->w = camera_rect.w;
	image_rect->h = camera_rect.h;
	SDL_BlitSurface(getCamera(), &camera_rect, displayImage_, image_rect);
	delete image_rect;

	std::string error_message = "Press any key to exit "+app_name_+" ...";
	FontTool::drawText((width_- FontTool::getTextWidth(error))/2,height_/2+60,error,displayImage_);
	FontTool::drawText((width_- FontTool::getTextWidth(error_message.c_str()))/2,height_/2+80,error_message.c_str(),displayImage_);
	SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
	SDL_RenderCopy(renderer_, display_, NULL, NULL);
 	SDL_RenderPresent(renderer_);

	error_=true;
	while(error_) {
		process_events();
		SDL_Delay(1);
	}
}

void SDLinterface::drawHelp(unsigned char* buffer)
{
	int y = FontTool::getFontHeight()-3;

	for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++) {
		if (strcmp(help_line->c_str(), "") == 0) y+=5;
		else {
			FontTool::drawText(FontTool::getFontHeight(),y,help_line->c_str(),displayImage_);
			y+=FontTool::getFontHeight()-3;
		}
	}

}

void SDLinterface::mainLoop()
{
	unsigned char* cameraReadBuffer = NULL;

	while(running_) {
		
		// do nothing if paused
		if (pause_){
			process_events();
			SDL_Delay(1);
			continue;
		}

		long start_time = currentTime();
		cameraReadBuffer = ringBuffer->getNextBufferToRead();
		// loop until we get access to a frame
		while (cameraReadBuffer==NULL) {
			cameraReadBuffer = ringBuffer->getNextBufferToRead();
			//if (cameraReadBuffer!=NULL) break;
			SDL_Delay(1);
			process_events();
			if(!running_) return;
		}
		long camera_time = currentTime();

		// do the actual image processing job
		for (frame = processorList.begin(); frame!=processorList.end(); frame++)
			(*frame)->process(cameraReadBuffer,destBuffer_,displayImage_);
		long processing_time = currentTime();

		// update display
		switch( displayMode_ ) {
			case NO_DISPLAY:
				break;
			case SOURCE_DISPLAY: {
				memcpy(sourceBuffer_,cameraReadBuffer,ringBuffer->size());
				SDL_UpdateTexture(texture_,NULL,sourceBuffer_,width_);
				SDL_RenderCopy(renderer_, texture_, NULL, NULL);
				if (help_) drawHelp(sourceBuffer_);
				camera_->drawGUI(displayImage_);
				SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
				SDL_RenderCopy(renderer_, display_, NULL, NULL);
				SDL_FillRect(displayImage_, NULL, 0 );
				SDL_RenderPresent(renderer_);
				break;
			}
			case DEST_DISPLAY: {
				SDL_UpdateTexture(texture_,NULL,destBuffer_,width_);
 				SDL_RenderCopy(renderer_, texture_, NULL, NULL);
				camera_->drawGUI(displayImage_);
				if (help_) drawHelp(destBuffer_);
                		SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
				SDL_RenderCopy(renderer_, display_, NULL, NULL);
				SDL_FillRect(displayImage_, NULL, 0 );
				SDL_RenderPresent(renderer_);
				break;
			}
		}

#ifndef NDEBUG
		if (recording_) {
			if (displayMode_!=SOURCE_DISPLAY) memcpy(sourceBuffer_,cameraReadBuffer,ringBuffer->size());
			saveBuffer(sourceBuffer_);
		}
#endif
		
		ringBuffer->readFinished();
		if (!recording_) frameStatistics(camera_time-start_time,processing_time-camera_time, currentTime()-start_time);
		process_events();
	}
}

void SDLinterface::endLoop() {
	// finish all FrameProcessors
	for (frame = processorList.begin(); frame!=processorList.end(); frame++)
		(*frame)->finish();

    if(error_) {
        showError("Camera disconnected!");
    }
}

void SDLinterface::frameStatistics(long cameraTime, long processingTime, long totalTime) {

	frames_++;
	processingTime_+=processingTime;
	totalTime_+=totalTime;
	//cameraTime_+=cameraTime;

	time_t currentTime;
	time(&currentTime);
	long diffTime = (long)( currentTime - lastTime_ );
	
    
    if ((fullscreen_) && (!calibrate_)) {
        char caption[24] = "";
        sprintf(caption,"%d FPS",current_fps);
        FontTool::drawText(width_-(FontTool::getTextWidth(caption)+FontTool::getFontHeight()),FontTool::getFontHeight(),caption,displayImage_);
    }
    
	if (diffTime >= 1) {
		current_fps = (int)floor( (frames_ / diffTime) + 0.5 );
		
		if (!calibrate_) {
            char caption[24] = "";
            sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps);
            SDL_SetWindowTitle( window_, caption);
		}

		/*std::cout 	<< floor((cameraTime_/frames_)/1000.0f) << " "
				<< floor((processingTime_/frames_)/1000.0f) << " "
				<< floor((totalTime_/frames_)/1000.0f) << std::endl;*/
		//std::cout << "frame latency: " << ((processingTime_/frames_)/100)/10.0f << " ms" << std::endl;

		//cameraTime_ = processingTime_ = totalTime_ = 0.0f;
		//std::cout << current_fps << std::endl;

		lastTime_ = (long)currentTime;
		frames_ = 0;
    }
}

char* checkRenderDriver(){

	SDL_RendererInfo drinfo;
	int numDrivers = SDL_GetNumRenderDrivers();
	const char *fallback = "software";

	for (int i=0; i<numDrivers; i++) {
		SDL_GetRenderDriverInfo (i, &drinfo);

		if ( (drinfo.flags & SDL_RENDERER_TARGETTEXTURE) && (drinfo.flags & SDL_RENDERER_ACCELERATED)) {
			return (char*)(drinfo.name);
		}
	}

	return (char*)fallback;
}

bool SDLinterface::setupWindow() {

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		printf("SDL could not be initialized: %s\n", SDL_GetError());
		return false;
	}

	if(headless_) return true;
	char *renderdriver = checkRenderDriver();
	printf("render: %s\n",renderdriver);

	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, renderdriver);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderdriver);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

 	SDL_CreateWindowAndRenderer(width_,height_,0,&window_,&renderer_);

	if ( window_ == NULL ) {
		printf("Could not open window: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	} else SDL_SetWindowTitle(window_,app_name_.c_str());

	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);

	/*iconImage_ = getIcon();
	#ifndef __APPLE__
	SDL_SetWindowIcon(window_, iconImage_);
	#endif*/

	FontTool::init();
	return true;
}

void SDLinterface::teardownWindow()
{
    if (!headless_) {
        FontTool::close();
        SDL_Quit();
    }
}

void SDLinterface::process_events()
{
    SDL_Event event;
    while( SDL_PollEvent( &event ) ) {

        switch( event.type ) {
		case SDL_KEYDOWN:
			if (error_) { error_ = false; return; }
			//printf("%d\n",event.key.keysym.sym);
            if( event.key.keysym.sym == SDLK_F1 ){
                //if (!calibrate_)
                toggleFullScreen();
            } else if( event.key.keysym.sym == SDLK_n ){
				displayMode_ = NO_DISPLAY;
				// turn the display black
				SDL_RenderClear(renderer_);
				SDL_RenderPresent(renderer_);
			} else if( event.key.keysym.sym == SDLK_s ){
				SDL_FillRect(displayImage_, NULL, 0 );
				displayMode_ = SOURCE_DISPLAY;
			} else if( event.key.keysym.sym == SDLK_t ){
				SDL_FillRect(displayImage_, NULL, 0 );
				displayMode_ = DEST_DISPLAY;
			} else if( event.key.keysym.sym == SDLK_o ){
				camera_->showSettingsDialog(); 
			} else if( event.key.keysym.sym == SDLK_v ){
				if (verbose_) {
					verbose_=false;
				} else {
					verbose_=true;
				}
			} 
#ifndef NDEBUG
			  else if( event.key.keysym.sym == SDLK_m ){
					if (recording_) {
						recording_ = false;
						char caption[24] = "";
						sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps);
						SDL_SetWindowTitle( window_, caption);
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
						std::string caption = app_name_ + " - recording";
						SDL_SetWindowTitle( window_, caption.c_str());
					}
			} else if( event.key.keysym.sym == SDLK_b ){
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
				if (displayMode_==DEST_DISPLAY) saveBuffer(destBuffer_);
				else saveBuffer(sourceBuffer_);
			} 
#endif
			else if( event.key.keysym.sym == SDLK_p ) {
				if (pause_) {
					pause_=false;
					char caption[24] = "";
					sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps);
					SDL_SetWindowTitle( window_, caption);
					
				} else {
					pause_=true;
					std::string caption = app_name_ + " - paused";
					SDL_SetWindowTitle( window_, caption.c_str());
					// turn the display black
					SDL_RenderClear(renderer_);
					SDL_RenderPresent(renderer_);
				}				
			} else if( event.key.keysym.sym == SDLK_c ) {
				if (calibrate_) {
					calibrate_=false;
					char caption[24] = "";
					sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps);
                    //if (!fullscreen_) SDL_SetWindowFullscreen(window_, 0);
                    SDL_SetWindowTitle( window_, caption);
				} else {
					calibrate_=true;
                    help_=false;
					std::string caption = app_name_ + " - calibration";
					SDL_SetWindowTitle( window_, caption.c_str());
                    //if (!fullscreen_) SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
				}				
			} else if( event.key.keysym.sym == SDLK_h ){
				help_=!help_;
			} else if( event.key.keysym.sym == SDLK_ESCAPE ){
				running_=false;
				return;
			} else {
                help_ = false;
            }

			for (frame = processorList.begin(); frame!=processorList.end(); frame++)
				(*frame)->toggleFlag(event.key.keysym.sym);
			camera_->control(event.key.keysym.sym);

			break;
		case SDL_QUIT:
			running_ = false;
			error_ = false;
			break;
        }
    }
}

void SDLinterface::allocateBuffers()
{
	bytesPerSourcePixel_ = sourceDepth_/8;
	bytesPerDestPixel_ = destDepth_/8;

	sourceBuffer_  = new unsigned char[3*width_*height_];
	destBuffer_    = new unsigned char[3*width_*height_];
	ringBuffer = new RingBuffer(width_*height_*bytesPerSourcePixel_);

    for (int i=0;i<3*width_*height_;i++) {
       	destBuffer_[i] = 128;
       	sourceBuffer_[i] = 128;
    }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 rmask = 0xff000000;
    Uint32 gmask = 0x00ff0000;
    Uint32 bmask = 0x0000ff00;
    Uint32 amask = 0x000000ff;
#else
    Uint32 rmask = 0x000000ff;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x00ff0000;
    Uint32 amask = 0xff000000;
#endif

    if(!headless_) {
        texture_ = SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STATIC,width_,height_);
        displayImage_ = SDL_CreateRGBSurface(0,width_,height_,32,rmask,gmask,bmask,amask);
        SDL_SetSurfaceBlendMode(displayImage_, SDL_BLENDMODE_BLEND);
        display_ = SDL_CreateTextureFromSurface(renderer_,displayImage_);

		SDL_FillRect(displayImage_, NULL, 0 );
       	SDL_RenderClear(renderer_);
       	SDL_RenderPresent(renderer_);
    }
}

void SDLinterface::freeBuffers()
{
	if (!headless_) {
		SDL_FreeSurface(displayImage_);
		//SDL_FreeSurface(iconImage_);

		SDL_DestroyTexture(display_);
		SDL_DestroyTexture(texture_);
		SDL_DestroyRenderer(renderer_);
		SDL_DestroyWindow(window_);
	}

	delete [] sourceBuffer_;
	delete [] destBuffer_;
	delete ringBuffer;
}

void SDLinterface::toggleFullScreen() {

    int flags = SDL_GetWindowFlags(window_);
    if(flags & SDL_WINDOW_FULLSCREEN) {
		SDL_ShowCursor(1);
        SDL_SetWindowFullscreen(window_, 0);
        fullscreen_ = false;
    } else {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_ShowCursor(0);
        fullscreen_ = true;
    }

}

void SDLinterface::addFrameProcessor(FrameProcessor *fp) {

	processorList.push_back(fp);
	fp->addMessageListener(this);
}


void SDLinterface::removeFrameProcessor(FrameProcessor *fp) {
	frame = std::find( processorList.begin(), processorList.end(), fp );
	if( frame != processorList.end() ) {
		processorList.erase( frame );
		fp->removeMessageListener(this);
	}
}

void SDLinterface::setMessage(std::string message)
{
	if (verbose_) {
		std::cout << message << std::endl;
		std::cout.flush();
	}
}

void SDLinterface::displayMessage(const char *message)
{
	SDL_RenderClear(renderer_);
	SDL_FillRect(displayImage_, NULL, 0 );
	FontTool::drawText((width_- FontTool::getTextWidth(message))/2,height_/2,message,displayImage_);
	SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
    SDL_RenderCopy(renderer_, display_, NULL, NULL);
    SDL_FillRect(displayImage_, NULL, 0 );
    SDL_RenderPresent(renderer_);
}

void SDLinterface::setDisplayMode(DisplayMode mode) {
	if ((window_!=NULL) && (mode==NO_DISPLAY)) {
		SDL_RenderClear(renderer_);
		SDL_RenderPresent(renderer_);
	}
	displayMode_ = mode;
}

void SDLinterface::initFrameProcessors() {
	for (frame = processorList.begin(); frame!=processorList.end(); ) {
		bool success = (*frame)->init(width_ , height_, bytesPerSourcePixel_, bytesPerDestPixel_);
		if(!success) {	
			processorList.erase( frame );
			printf("removed frame processor\n");
		} else frame++;
	}
}

unsigned int SDLinterface::current_fps = 0;
bool SDLinterface::display_lock = false;

SDLinterface::SDLinterface(const char* name, CameraEngine *camera, reactivision_settings *config)
	: error_( false )
	, pause_( false )
	, calibrate_( false )
	, help_( false )
	, framenumber_( 0 )
	, frames_( 0 )
	, recording_( false )
	, width_( WIDTH )
	, height_( HEIGHT )
	, displayMode_( DEST_DISPLAY )
{
    camera_ = camera;
	cameraTime_ = processingTime_ = totalTime_ = 0.0f;
	headless_ = config->headless;
    fullscreen_ = config->fullscreen;
	
	time_t start_time;
	time(&start_time);
	lastTime_ = (long)start_time;

	app_name_ = std::string(name);
	sourceDepth_ = 8;
	destDepth_   = 8;

	window_ = NULL;
	sourceBuffer_ = NULL;
	destBuffer_ = NULL;
		
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



