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

#include "SDLinterface.h"
#include "VisionEngine.h"


unsigned int fontPixel(SDL_Surface *surface, int x, int y)
{
	return *((unsigned char*)surface->pixels + y * surface->pitch + x);
}

void SDLinterface::initFont()
{
	font_ = (SdlFont *) malloc(sizeof(SdlFont));
	font_->Surface = getFont();
	
	SDL_LockSurface(font_->Surface);
	
	unsigned char green = SDL_MapRGB(font_->Surface->format, 255, 0, 255);
	int x = 0, i = 0;
	while (x < font_->Surface->w) {
		if (fontPixel(font_->Surface, x, 0) == green) {
			font_->CharPos[i++]=x;
			while((x < font_->Surface->w) && (fontPixel(font_->Surface, x, 0)==green))
				x++;
			font_->CharPos[i++]=x;
		}
		x++;
	}
	font_->MaxPos = x-1;
	
	unsigned char pixel = fontPixel(font_->Surface, 0, font_->Surface->h-1);
	SDL_UnlockSurface(font_->Surface);
	SDL_SetColorKey(font_->Surface, SDL_TRUE, pixel);
	
	font_->Texture = SDL_CreateTextureFromSurface(renderer_,font_->Surface);
}

void SDLinterface::freeFont()
{
	SDL_DestroyTexture(font_->Texture);
	SDL_FreeSurface(font_->Surface);
	free(font_);
}

void SDLinterface::drawText(int xpos, int ypos, const char* text)
{
	const char* c;
	int charoffset;
	SDL_Rect srcrect, dstrect;
	
	if(text == NULL) return;
	
	// these values won't change in the loop
	srcrect.y = 1;
	dstrect.y = ypos;
	srcrect.h = dstrect.h = textHeight();
	
	SDL_SetRenderTarget(renderer_,display_);
	
	for(c = text; *c != '\0' && xpos <= width_ ; c++) {
		charoffset = ((int) (*c - 33)) * 2 + 1;
		// skip spaces and non printable characters
		if (*c == ' ' || charoffset < 0 || charoffset > font_->MaxPos) {
			xpos += font_->CharPos[2]-font_->CharPos[1];
			continue;
		}
		
		srcrect.w = dstrect.w =
		(font_->CharPos[charoffset+2] + font_->CharPos[charoffset+1])/2 -
		(font_->CharPos[charoffset] + font_->CharPos[charoffset-1])/2;
		srcrect.x = (font_->CharPos[charoffset]+font_->CharPos[charoffset-1])/2;
		dstrect.x = xpos - (float)(font_->CharPos[charoffset]
								   - font_->CharPos[charoffset-1])/2;
		
		SDL_RenderCopy(renderer_, font_->Texture, &srcrect, &dstrect);
		
		xpos += font_->CharPos[charoffset+1] - font_->CharPos[charoffset];
	}
	
	SDL_SetRenderTarget(renderer_,NULL);
}

int SDLinterface::textWidth(const char *text)
{
	const char* c;
	int charoffset=0;
	int w = 0;
	
	if(text == NULL) return 0;
	
	for(c = text; *c != '\0'; c++) {
		charoffset = ((int) *c - 33) * 2 + 1;
		// skip spaces and non printable characters
		if (*c == ' ' || charoffset < 0 || charoffset > font_->MaxPos) {
			w += font_->CharPos[2]-font_->CharPos[1];
			continue;
		}
		
		w += font_->CharPos[charoffset+1] - font_->CharPos[charoffset];
	}
	
	return w;
}

int SDLinterface::textHeight()
{
	return font_->Surface->h - 1;
}

// the principal program sequence
bool SDLinterface::openDisplay(VisionEngine *engine)
{
	engine_ = engine;
	if( !setupWindow() ) return false;
	SDL_RenderClear(renderer_);
	if (displayMode_==NO_DISPLAY) SDL_RenderPresent(renderer_);
	else displayMessage("starting camera");
	select_ = false;
	return true;
}

unsigned char* SDLinterface::getDisplay()
{
	return NULL;
}

void SDLinterface::closeDisplay()
{
	error_ = false;
	freeBuffers();
}

void SDLinterface::updateDisplay()
{
	
	if (select_)
	{
		SDL_SetRenderDrawColor(renderer_,0,0,0,0);
		SDL_RenderClear(renderer_);
		SDL_SetRenderTarget(renderer_, display_);
		SDL_RenderClear(renderer_);
				
		int y = textHeight()-3;
		drawText(textHeight(),1*y,"UP/DOWN - select camera setting");
		drawText(textHeight(),2*y,"ENTER   - apply camera selection");
		drawText(textHeight(),3*y,"K       - cancel camera selection");
		
		CameraConfig *cfg = &dev_list[selector_];
		char camera_str[256];
		sprintf(camera_str,"%s_%d: %s",dstr[cfg->driver],cfg->device,cfg->name);
		drawText((width_- textWidth(camera_str))/2,height_/2-textHeight(),camera_str);
		
		char format_str[256];
		if (cfg->frame_mode>=0) {
			sprintf(format_str,"format7_%d %dx%d (%s)",cfg->frame_mode, cfg->cam_width,cfg->cam_height,fstr[cfg->cam_format]);
		} else if (cfg->cam_fps==(int)cfg->cam_fps) sprintf(format_str,"%dx%d@%d (%s)",cfg->cam_width,cfg->cam_height,(int)cfg->cam_fps,fstr[cfg->cam_format]);
		else sprintf(format_str,"%dx%d@%.1f (%s)",cfg->cam_width,cfg->cam_height,cfg->cam_fps,fstr[cfg->cam_format]);
		drawText((width_- textWidth(format_str))/2,height_/2,format_str);
		SDL_RenderCopy(renderer_, display_, NULL, NULL);
		SDL_RenderPresent(renderer_);
		
		SDL_Delay(10);
		return;
	}
	
	// update display
	switch( displayMode_ )
	{
		case NO_DISPLAY:
			break;
		case SOURCE_DISPLAY:
		{
			SDL_SetRenderTarget(renderer_, NULL);
			if (format_==FORMAT_RGB) SDL_UpdateTexture(texture_,NULL,sourceBuffer_,width_*format_);
			else SDL_UpdateYUVTexture(texture_,NULL,sourceBuffer_,width_*format_,uvBuffer_,width_*format_,uvBuffer_,width_*format_);
			SDL_RenderCopy(renderer_, texture_, NULL, NULL);
			if (help_) drawHelp();
			SDL_RenderCopy(renderer_, display_, NULL, NULL);
			SDL_SetRenderTarget(renderer_, display_);
			SDL_SetRenderDrawColor(renderer_,0,0,0,0);
			SDL_RenderClear(renderer_);
			SDL_SetRenderTarget(renderer_, NULL);
			SDL_RenderPresent(renderer_);
			break;
		}
		case DEST_DISPLAY:
		{
			SDL_SetRenderTarget(renderer_, NULL);
			if (format_==FORMAT_RGB) SDL_UpdateTexture(texture_,NULL,destBuffer_,width_*format_);
			else SDL_UpdateYUVTexture(texture_,NULL,destBuffer_,width_*format_,uvBuffer_,width_*format_,uvBuffer_,width_*format_);
			SDL_RenderCopy(renderer_, texture_, NULL, NULL);
			if (help_) drawHelp();
			SDL_RenderCopy(renderer_, display_, NULL, NULL);
			SDL_SetRenderTarget(renderer_, display_);
			SDL_SetRenderDrawColor(renderer_,0,0,0,0);
			SDL_RenderClear(renderer_);
			SDL_SetRenderTarget(renderer_, NULL);
			SDL_RenderPresent(renderer_);
			break;
		}
	}
	
	processEvents();
	showFrameRate();
}

char* SDLinterface::checkRenderDriver()
{
	
	SDL_RendererInfo drinfo;
	int numDrivers = SDL_GetNumRenderDrivers();
	const char *fallback = "software";
	
	for (int i=0; i<numDrivers; i++)
	{
		SDL_GetRenderDriverInfo (i, &drinfo);
		
		if ( (drinfo.flags & SDL_RENDERER_TARGETTEXTURE) && (drinfo.flags & SDL_RENDERER_ACCELERATED))
		{
			return (char*)(drinfo.name);
		}
	}
	
	return (char*)fallback;
}

bool SDLinterface::setupWindow()
{
	
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		//printf("SDL could not be initialized: %s\n", SDL_GetError());
		printf("SDLinterface not available!\n");
		return false;
	}
	
	char *renderdriver = checkRenderDriver();
	printf("render: %s\n\n",renderdriver);
	
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, renderdriver);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderdriver);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
	
	SDL_CreateWindowAndRenderer(width_,height_,0,&window_,&renderer_);
	
	if ( window_ == NULL )
	{
		printf("Could not open window: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}
	
	if(fullscreen_)
	{
		SDL_ShowCursor(0);
		SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	
	SDL_SetWindowTitle(window_,app_name_.c_str());

	SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);
	SDL_EventState(SDL_FINGERMOTION,SDL_IGNORE);
	SDL_EventState(SDL_FINGERDOWN,SDL_IGNORE);
	SDL_EventState(SDL_FINGERUP,SDL_IGNORE);
	SDL_EventState(SDL_KEYUP,SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT,SDL_IGNORE);
	//SDL_EventState(SDL_WINDOWEVENT,SDL_IGNORE);

	allocateBuffers();
	processEvents();
	
	return true;
}

void SDLinterface::selectCamera()
{
	
	if (select_)
	{
		engine_->resetCamera();
		select_ = false;
		return;
	}
	
	engine_->teardownCamera();

	dev_list = CameraTool::findDevices();
	if (dev_list.size()==0) {
		displayError("No camera found!");
		return;
	}

	selector_ = 0;
	std::string caption = app_name_ + " - select camera";
	SDL_SetWindowTitle( window_, caption.c_str());
	
	select_ = true;
}

void SDLinterface::processEvents()
{
	SDL_Event event;
	while( SDL_PollEvent( &event ) )
	{
		
		switch( event.type )
		{
				
			case SDL_KEYDOWN:
				if (error_)
				{
					error_ = false;
					return;
				}
				//printf("%d\n",event.key.keysym.sym);
				if( event.key.keysym.sym == SDLK_F1 )
				{
					//if (!calibrate_)
					if (!event.key.repeat) toggleFullScreen();
				}
				else if( event.key.keysym.sym == SDLK_n )
				{
					displayMode_ = NO_DISPLAY;
					// turn the display black
					SDL_RenderClear(renderer_);
					SDL_RenderPresent(renderer_);
				}
				else if( event.key.keysym.sym == SDLK_s )
				{
					displayMode_ = SOURCE_DISPLAY;
				}
				else if( event.key.keysym.sym == SDLK_t )
				{
					displayMode_ = DEST_DISPLAY;
				}
				else if( event.key.keysym.sym == SDLK_v )
				{
					if (verbose_)
					{
						verbose_=false;
					}
					else
					{
						verbose_=true;
					}
				}
				else if( event.key.keysym.sym == SDLK_p )
				{
					if (pause_)
					{
						engine_->pause(pause_=false);
						char caption[24] = "";
						sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps_);
						SDL_SetWindowTitle( window_, caption);
					}
					else
					{
						engine_->pause(pause_=true);
						std::string caption = app_name_ + " - paused";
						SDL_SetWindowTitle( window_, caption.c_str());
#ifndef NDEBUG
						// turn the display black
						SDL_RenderClear(renderer_);
						SDL_RenderPresent(renderer_);
#endif
					}
				}
				else if( event.key.keysym.sym == SDLK_k )
				{
					selectCamera();
				}
				else if( event.key.keysym.sym == SDLK_RETURN )
				{
					if (select_) engine_->resetCamera(&dev_list[selector_]);
				}
				else if( event.key.keysym.sym == SDLK_UP )
				{
					if (select_)
					{
						selector_--;
						if (selector_<0) selector_=dev_list.size()-1;
					}
				}
				else if( event.key.keysym.sym == SDLK_DOWN )
				{
					if (select_)
					{
						selector_++;
						if (selector_==(int)dev_list.size()) selector_=0;
					}
				}
				else if( event.key.keysym.sym == SDLK_h )
				{
					help_=!help_;
				}
				else if( event.key.keysym.sym == SDLK_ESCAPE )
				{
					engine_->stop();
					return;
				}
				
#ifndef NDEBUG
				else if( event.key.keysym.sym == SDLK_m )
				{
					if (recording_)
					{
						recording_ = false;
						char caption[24] = "";
						sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps_);
						SDL_SetWindowTitle( window_, caption);
					}
					else
					{
						recording_ = true;
						char caption[24] = "";
						sprintf(caption,"recording - %d FPS",current_fps_);
						SDL_SetWindowTitle( window_, caption);
					}
				}
				else help_ = false;
#endif
				if (!select_) engine_->event(event.key.keysym.scancode);
				break;
			case SDL_QUIT:
				engine_->stop();
				return;
			/*default:
				std::cout << event.type << std::endl;
				break;*/
		}
	}
	if (select_) updateDisplay();
}

void SDLinterface::setBuffers(unsigned char *src, unsigned char *dest, int width, int height, int format)
{
	width_ = width;
	height_ = height;
	format_ = format;
	
	sourceBuffer_  = src;
	destBuffer_    = dest;
}

void SDLinterface::allocateBuffers()
{
	if (format_==FORMAT_RGB) texture_ = SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STATIC,width_,height_);
	else
	{
		texture_ = SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STATIC,width_,height_);
		uvBuffer_ = new unsigned char [width_*height_];
		for (int i=0; i<width_*height_; i++) uvBuffer_[i] = 128;
	}
	
	display_ =  SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGBA8888 , SDL_TEXTUREACCESS_TARGET ,width_,height_);
	SDL_SetTextureBlendMode(display_, SDL_BLENDMODE_BLEND);
	
	initFont();
}

void SDLinterface::freeBuffers()
{
	SDL_DestroyTexture(display_);
	SDL_DestroyTexture(texture_);
	SDL_DestroyRenderer(renderer_);
	SDL_DestroyWindow(window_);
	freeFont();
}

void SDLinterface::toggleFullScreen()
{
	if(fullscreen_)
	{
		SDL_SetWindowFullscreen(window_, SDL_FALSE);
		SDL_SetWindowSize(window_, width_, height_);
		SDL_ShowCursor(1);
		fullscreen_ = false;
	}
	else
	{
		SDL_ShowCursor(0);
		SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
		if (displayMode_==NO_DISPLAY) displayMode_ = DEST_DISPLAY;
		fullscreen_ = true;
	}

}

void SDLinterface::showFrameRate()
{
	
	frames_++;
	long currentTime_ = VisionEngine::currentSeconds();
	long diffTime = currentTime_ - lastTime_;
	
	if (fullscreen_)
	{
		char caption[24] = "";
		sprintf(caption,"%d FPS",current_fps_);
		drawText(width_-(textWidth(caption)+textHeight()),textHeight(),caption);
	}
	
	if (diffTime >= 1)
	{
		current_fps_ = (int)floor( (frames_ / diffTime) + 0.5 );
		
		char caption[24] = "";
		sprintf(caption,"%s - %d FPS",app_name_.c_str(),current_fps_);
#ifndef NDEBUG
		if (recording_) sprintf(caption,"recording - %d FPS",current_fps_);
#endif
		SDL_SetWindowTitle( window_, caption);
		
		lastTime_ = currentTime_;
		frames_ = 0;
	}
}

void SDLinterface::printMessage(std::string message)
{
	if (verbose_)
	{
		std::cout << message << std::endl;
		std::cout.flush();
	}
}

void SDLinterface::displayError(const char* error)
{
	SDL_SetRenderDrawColor(renderer_,0,0,0,0);
	SDL_SetRenderTarget(renderer_, display_);
	SDL_RenderClear(renderer_);
	SDL_SetRenderTarget(renderer_, NULL);
	SDL_RenderClear(renderer_);
	
	SDL_Surface *cam_surface = getCamera();
	SDL_Rect cam_rect = {width_/2-cam_surface->w/2,height_/2-cam_surface->h/2,cam_surface->w,cam_surface->h};
	SDL_Texture *cam_texture = SDL_CreateTextureFromSurface(renderer_,cam_surface);
	SDL_RenderCopy(renderer_, cam_texture, NULL, &cam_rect);
	
	drawText((width_- textWidth(error))/2,height_/2+60,error);
	std::string error_message = "Press any key to exit "+app_name_+" ...";
	drawText((width_- textWidth(error_message.c_str()))/2,height_/2+80,error_message.c_str());
	
	SDL_RenderCopy(renderer_, display_, NULL, NULL);
	SDL_RenderPresent(renderer_);
	SDL_SetWindowTitle(window_,app_name_.c_str());
	
	SDL_FreeSurface(cam_surface);
	SDL_DestroyTexture(cam_texture);
	
	error_=true;
	while(error_)
	{
		processEvents();
		SDL_Delay(1);
	}
}

void SDLinterface::drawHelp()
{
	int y = textHeight()-3;
	
	for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++)
	{
		if (strcmp(help_line->c_str(), "") == 0) y+=5;
		else
		{
			drawText(textHeight(),y,help_line->c_str());
			y+=textHeight()-3;
		}
	}
}

void SDLinterface::displayMessage(const char *message)
{
	SDL_SetRenderDrawColor(renderer_,0,0,0,0);
	SDL_SetRenderTarget(renderer_, display_);
	SDL_RenderClear(renderer_);
	
	drawText((width_- textWidth(message))/2,height_/2,message);
	
	SDL_RenderCopy(renderer_, display_, NULL, NULL);
	SDL_RenderPresent(renderer_);
}

void SDLinterface::displayControl(const char *title, int min, int max, int value)
{
	if (displayMode_==NO_DISPLAY) return;
	
	int y = textHeight()-3;
	drawText(textHeight(),2*y,"LEFT/RIGHT - adjust configuration value");
	drawText(textHeight(),3*y,"UP/DOWN    - switch to alternative option");
	
	int x_offset=width_/2-128;
	int y_offset=height_-100;
	int size = (int)(254*(((float)value-(float)min)/(float)(max-min)));
	
	// draw the title
	drawText(x_offset+128-(textWidth(title)/2),y_offset-textHeight(),title);
	
	SDL_SetRenderTarget(renderer_, display_);
	SDL_SetRenderDrawColor(renderer_,0,255,0,255);
	
	// draw the border
	SDL_Rect border = {x_offset,y_offset,256,25};
	SDL_RenderDrawRect(renderer_,&border);
	// draw the bar
	SDL_Rect bar = {x_offset+1,y_offset+1,size,23};
	SDL_RenderFillRect(renderer_,&bar);
	
	SDL_SetRenderTarget(renderer_, NULL);
	SDL_SetRenderDrawColor(renderer_,0,0,0,0);
}

void SDLinterface::setColor(unsigned char r, unsigned char g, unsigned char b)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	SDL_SetRenderDrawColor(renderer_,r,g,b,255);
}

void SDLinterface::drawPoint(int x,int y)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	SDL_RenderDrawPoint(renderer_, x, y);
}

void SDLinterface::drawLine(int x1,int y1, int x2, int y2)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
}

void SDLinterface::drawRect(int x,int y, int w, int h)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	SDL_Rect rect = {x,y,w,h};
	SDL_RenderDrawRect(renderer_, &rect);
}

void SDLinterface::drawRect(int x,int y, int w, int h, float r)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);

	int w0 = w/2;
	int h0 = h/2;
	int w1 = -1*w0;
	int h1 = -1*h0;
	
	double Cr = cos(r);
	double Sr = sin(r);

	int x0 = x + (int)floor(w0*Cr - h0*Sr + 0.5f);
	int y0 = y + (int)floor(h0*Cr + w0*Sr + 0.5f);

	int x1 = x + (int)floor(w1*Cr - h0*Sr + 0.5f);
	int y1 = y + (int)floor(h0*Cr + w1*Sr + 0.5f);

	int x2 = x + (int)floor(w1*Cr - h1*Sr + 0.5f);
	int y2 = y + (int)floor(h1*Cr + w1*Sr + 0.5f);

	int x3 = x + (int)floor(w0*Cr - h1*Sr + 0.5f);
	int y3 = y + (int)floor(h1*Cr + w0*Sr + 0.5f);

	SDL_RenderDrawLine(renderer_, x0, y0, x1, y1);
	SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
	SDL_RenderDrawLine(renderer_, x2, y2, x3, y3);
	SDL_RenderDrawLine(renderer_, x3, y3, x0, y0);
}

void SDLinterface::fillRect(int x,int y, int w, int h)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	SDL_Rect rect = {x,y,w,h};
	SDL_RenderFillRect(renderer_, &rect);
}

void SDLinterface::drawEllipse(int x,int y, int w, int h)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	
	float step = 0.002 * width_/w;
	
	for (double a=M_PI/2; a>0; a-=step)
	{
		double X = cos(a)*w/2.0f;
		double Y = sin(a)*h/2.0f;
		drawPoint(x+X,y+Y);
		drawPoint(x-X,y+Y);
		drawPoint(x+X,y-Y);
		drawPoint(x-X,y-Y);
	}
}

void SDLinterface::drawEllipse(int x,int y, int w, int h, float r)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	
	float step = 0.002 * width_/w;
	
	for (double a=2*M_PI; a>0; a-=step)
	{
		double X = cos(a)*w/2.0f;
		double Y = sin(a)*h/2.0f;
		
		double Cr = cos(r);
		double Sr = sin(r);
		
		double Xr = X*Cr - Y*Sr;
		double Yr = Y*Cr + X*Sr;

		drawPoint(x+Xr,y+Yr);
	}
}

void SDLinterface::fillEllipse(int x,int y, int w, int h)
{
	if (displayMode_==NO_DISPLAY) return;
	if (!SDL_GetRenderTarget(renderer_)) SDL_SetRenderTarget(renderer_, display_);
	
	float step = 0.002 * width_/w;
	
	for (double a=M_PI/2; a>-M_PI/2; a-=step)
	{
		double X = cos(a)*w/2.0f;
		double Y = sin(a)*h/2.0f;
		drawLine(x-X,y+Y,x+X,y+Y);
	}
}

void SDLinterface::setDisplayMode(DisplayMode mode)
{
	if ((window_!=NULL) && (mode==NO_DISPLAY))
	{
		SDL_RenderClear(renderer_);
		SDL_RenderPresent(renderer_);
	}
	displayMode_ = mode;
}

void SDLinterface::setHelpText(std::vector<std::string> hlp)
{
	
	for(std::vector<std::string>::iterator help_line = hlp.begin(); help_line!=hlp.end(); help_line++)
	{
		help_text.push_back(*help_line);
	}
	
	//print the help message
	for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++)
	{
		std::cout << *help_line << std::endl;
	}
	std::cout << std::endl;
}

SDLinterface::SDLinterface(const char* name, bool fullscreen)
: error_( false )
, pause_( false )
, help_( false )
, select_( false )
#ifndef NDEBUG
, recording_(false )
#endif
, frames_( 0 )
, current_fps_( 0 )
{
	displayMode_ = DEST_DISPLAY;
	app_name_ = std::string(name);
	fullscreen_ = fullscreen;
	
	lastTime_ = VisionEngine::currentSeconds();
	cameraTime_ = processingTime_ = totalTime_ = 0.0f;
	
	window_ = NULL;
	sourceBuffer_ = NULL;
	destBuffer_ = NULL;
	uvBuffer_ = NULL;
	
	help_text.push_back("camera:");
	help_text.push_back("   o - camera options");
	help_text.push_back("   k - select camera");
	help_text.push_back("display:");
	help_text.push_back("   n - no image");
	help_text.push_back("   s - source image");
	help_text.push_back("   t - target image");
	help_text.push_back("   h - toggle help text");
	help_text.push_back("  F1 - toggle fullscreen");
	help_text.push_back("");
	help_text.push_back("control:");
	help_text.push_back("   v - verbose output");
	help_text.push_back("   p - pause processing");
	help_text.push_back("   ESC - quit " + app_name_);


#ifndef NDEBUG
	help_text.push_back("");
	help_text.push_back("debug options:");
	help_text.push_back("   m - save buffer as PGM sequence");
	help_text.push_back("   n - save buffer as PGM image");
#endif
}



