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

#include "SDLinterface.h"
#include "VisionEngine.h"

// the principal program sequence
unsigned char* SDLinterface::openDisplay(VisionEngine *engine) {
    engine_ = engine;
    if( !setupWindow() ) return NULL;
    if (displayMode_==NO_DISPLAY) {
        SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);
    } else displayMessage("starting camera");
    return (unsigned char*)displayImage_->pixels;
}

void SDLinterface::closeDisplay()
{
    engine_->stop();
    freeBuffers();
}

void SDLinterface::updateDisplay()
{
    // update display
    switch( displayMode_ ) {
        case NO_DISPLAY:
            break;
        case SOURCE_DISPLAY: {
            //memcpy(sourceBuffer_,cameraReadBuffer,ringBuffer->size());
            SDL_UpdateTexture(texture_,NULL,sourceBuffer_,width_*bytesPerSourcePixel_);
            SDL_RenderCopy(renderer_, texture_, NULL, NULL);
            if (help_) drawHelp(sourceBuffer_);
            //camera_->drawGUI(this);
            SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
            SDL_RenderCopy(renderer_, display_, NULL, NULL);
            SDL_FillRect(displayImage_, NULL, 0 );
            SDL_RenderPresent(renderer_);
            break;
        }
        case DEST_DISPLAY: {
            SDL_UpdateTexture(texture_,NULL,destBuffer_,width_*bytesPerDestPixel_);
            SDL_RenderCopy(renderer_, texture_, NULL, NULL);
            //camera_->drawGUI(this);
            if (help_) drawHelp(destBuffer_);
            SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
            SDL_RenderCopy(renderer_, display_, NULL, NULL);
            SDL_FillRect(displayImage_, NULL, 0 );
            SDL_RenderPresent(renderer_);
            break;
        }
    }
    
    processEvents();
    showFrameRate();
}

char* SDLinterface::checkRenderDriver() {
    
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
    }
    
    if(fullscreen_) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(0);
    }

    SDL_SetWindowTitle(window_,app_name_.c_str());
    allocateBuffers();
    processEvents();
    
    return true;
}

void SDLinterface::processEvents()
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
                } else if( event.key.keysym.sym == SDLK_v ){
                    if (verbose_) {
                        verbose_=false;
                    } else {
                        verbose_=true;
                    }
                } else if( event.key.keysym.sym == SDLK_p ) {
                    if (pause_) {
                        engine_->pause(pause_=false);
                        char caption[24] = "";
                        sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps_);
                        SDL_SetWindowTitle( window_, caption);
                    } else {
                        engine_->pause(pause_=true);
                        std::string caption = app_name_ + " - paused";
                        SDL_SetWindowTitle( window_, caption.c_str());
                        // turn the display black
                        SDL_RenderClear(renderer_);
                        SDL_RenderPresent(renderer_);
                    }
                } else if( event.key.keysym.sym == SDLK_h ){
                    help_=!help_;
                } else if( event.key.keysym.sym == SDLK_ESCAPE ){
                    closeDisplay();
                    return;
                }
                
#ifndef NDEBUG
                else if( event.key.keysym.sym == SDLK_m ){
                    if (recording_) {
                        recording_ = false;
                        char caption[24] = "";
                        sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps_);
                        SDL_SetWindowTitle( window_, caption);
                    } else {
                        recording_ = true;
                        std::string caption = app_name_ + " - recording";
                        SDL_SetWindowTitle( window_, caption.c_str());
                    }
                }
#endif
                else {
                    help_ = false;
                    engine_->event(event.key.keysym.scancode);
                }
                
                break;
            case SDL_QUIT:
                closeDisplay();
                return;
        }
    }
    
}

void SDLinterface::setBuffers(unsigned char *src, unsigned char *dest, int width, int height, bool color)
{
    if (color) {
        bytesPerSourcePixel_ = 3;
        bytesPerDestPixel_ = 3;
    } else {
        bytesPerSourcePixel_ = 1;
        bytesPerDestPixel_ = 1;
    }
    
    width_ = width;
    height_ = height;
    
    sourceBuffer_  = src;
    destBuffer_    = dest;
    
    for (int i=0;i<3*width_*height_;i++) {
       	destBuffer_[i] = 128;
       	sourceBuffer_[i] = 128;
    }

}

void SDLinterface::allocateBuffers()
{
    
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
    
    if (bytesPerSourcePixel_==3) texture_ = SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STATIC,width_,height_);
    else texture_ = SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STATIC,width_,height_);
    
    displayImage_ = SDL_CreateRGBSurface(0,width_,height_,32,rmask,gmask,bmask,amask);
    SDL_SetSurfaceBlendMode(displayImage_, SDL_BLENDMODE_BLEND);
    display_ = SDL_CreateTextureFromSurface(renderer_,displayImage_);
    
    FontTool::init(displayImage_);
}

void SDLinterface::freeBuffers()
{
    SDL_FreeSurface(displayImage_);
    SDL_DestroyTexture(display_);
    SDL_DestroyTexture(texture_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    FontTool::close();
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

void SDLinterface::showFrameRate() {
    
    frames_++;
    long currentTime_ = VisionEngine::currentSeconds();
    long diffTime = currentTime_ - lastTime_;
    
    if (fullscreen_) {
        char caption[24] = "";
        sprintf(caption,"%d FPS",current_fps_);
        FontTool::drawText(width_-(FontTool::getTextWidth(caption)+FontTool::getFontHeight()),FontTool::getFontHeight(),caption);
    }
    
    if (diffTime >= 1) {
        current_fps_ = (int)floor( (frames_ / diffTime) + 0.5 );
        
        char caption[24] = "";
        sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps_);
        SDL_SetWindowTitle( window_, caption);
        
        lastTime_ = currentTime_;
        frames_ = 0;
    }
}

void SDLinterface::printMessage(std::string message)
{
    if (verbose_) {
        std::cout << message << std::endl;
        std::cout.flush();
    }
}

void SDLinterface::displayError(const char* error)
{
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
    FontTool::drawText((width_- FontTool::getTextWidth(error))/2,height_/2+60,error);
    FontTool::drawText((width_- FontTool::getTextWidth(error_message.c_str()))/2,height_/2+80,error_message.c_str());
    SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
    SDL_RenderCopy(renderer_, display_, NULL, NULL);
    SDL_RenderPresent(renderer_);
    
    error_=true;
    while(error_) {
        processEvents();
        SDL_Delay(1);
    }
}

void SDLinterface::drawHelp(unsigned char* buffer)
{
    int y = FontTool::getFontHeight()-3;
    
    for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++) {
        if (strcmp(help_line->c_str(), "") == 0) y+=5;
        else {
            FontTool::drawText(FontTool::getFontHeight(),y,help_line->c_str());
            y+=FontTool::getFontHeight()-3;
        }
    }
}

void SDLinterface::displayMessage(const char *message)
{
    SDL_RenderClear(renderer_);
    SDL_FillRect(displayImage_, NULL, 0 );
    FontTool::drawText((width_- FontTool::getTextWidth(message))/2,height_/2,message);
    SDL_UpdateTexture(display_,NULL,displayImage_->pixels,displayImage_->pitch);
    SDL_RenderCopy(renderer_, display_, NULL, NULL);
    SDL_FillRect(displayImage_, NULL, 0 );
    SDL_RenderPresent(renderer_);
}

void SDLinterface::displayControl(const char *title, int min, int max, int value)
{
    
    unsigned char* disp = (unsigned char*)(displayImage_->pixels);
    
    int x_offset=width_/2-128;
    int y_offset=height_-100;
    int range = max - min;
    
    FontTool::drawText(x_offset+128-(FontTool::getTextWidth(title)/2),y_offset-FontTool::getFontHeight(),title);
    
    // draw the border
    for (int i=x_offset;i<(x_offset+256);i++) {
        int pixel=(width_*y_offset+i)*4+1;
        disp[pixel]=disp[pixel+2]=255;
        pixel=(width_*(y_offset+25)+i)*4+1;
        disp[pixel]=disp[pixel+2]=255;
    }
    for (int i=y_offset;i<(y_offset+25);i++) {
        int pixel=(width_*i+x_offset)*4+1;
        disp[pixel]=disp[pixel+2]=255;
        pixel=(width_*i+x_offset+256)*4+1;
        disp[pixel]=disp[pixel+2]=255;
    }
    
    // draw the bar
    int xpos = (int)(254*(((float)value-(float)min)/(float)range));
    for (int i=x_offset+2;i<=(x_offset+xpos);i++) {
        for (int j=y_offset+2;j<=(y_offset+23);j++) {
            int pixel=(width_*j+i)*4+1;
            disp[pixel]=disp[pixel+2]=255;
        }
    }
    
}

void SDLinterface::drawMark(int xpos, int ypos, const char *mark, int state) {

    int pixel = 0;
    unsigned char* disp = (unsigned char*)(displayImage_->pixels);
    for (int i=1;i<3;i++) {
        pixel=4*(ypos*width_+(xpos+i));
        if ((pixel>=0) && (pixel<width_*height_*3)) disp[pixel+state]=disp[pixel+3]=255;
        pixel=4*(ypos*width_+(xpos-i));
        if ((pixel>=0) && (pixel<width_*height_*3)) disp[pixel+state]=disp[pixel+3]=255;
        pixel=4*((ypos+i)*width_+xpos);
        if ((pixel>=0) && (pixel<width_*height_*3)) disp[pixel+state]=disp[pixel+3]=255;
        pixel=4*((ypos-i)*width_+xpos);
        if ((pixel>=0) && (pixel<width_*height_*3)) disp[pixel+state]=disp[pixel+3]=255;
    }
    
    FontTool::drawText(xpos,ypos,mark);
}

void SDLinterface::setDisplayMode(DisplayMode mode) {
    if ((window_!=NULL) && (mode==NO_DISPLAY)) {
        SDL_RenderClear(renderer_);
        SDL_RenderPresent(renderer_);
    }
    displayMode_ = mode;
}

SDLinterface::SDLinterface(const char* name, bool fullscreen)
: error_( false )
, pause_( false )
, help_( false )
#ifndef NDEBUG
, recording_(false)
#endif
, frames_( 0 )
, current_fps_( 0 )
, width_( WIDTH )
, height_( HEIGHT )

{
    displayMode_ = DEST_DISPLAY;
    app_name_ = std::string(name);
    fullscreen_ = fullscreen;
    
    lastTime_ = VisionEngine::currentSeconds();
    cameraTime_ = processingTime_ = totalTime_ = 0.0f;
    
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



