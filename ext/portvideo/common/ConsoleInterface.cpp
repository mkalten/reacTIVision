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

#include "ConsoleInterface.h"
#include "VisionEngine.h"

// the principal program sequence
unsigned char* ConsoleInterface::openDisplay(VisionEngine *engine) {
	engine_ = engine;
	
	tcgetattr(STDIN_FILENO, &term_cfg);
	termios new_cfg = term_cfg;
	new_cfg.c_lflag &= ~ECHO;
	new_cfg.c_lflag &= ~ICANON;
	new_cfg.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_cfg);
	
	tv.tv_sec = 0;
	tv.tv_usec = 10;
	
	displayBuffer_ = new unsigned char[4*width_*height_];
    return displayBuffer_;
}

void ConsoleInterface::closeDisplay()
{
}

void ConsoleInterface::updateDisplay()
{
    processEvents();
    if (verbose_) printFrameRate();
}

void ConsoleInterface::processEvents()
{
	FD_ZERO(&readset);
	FD_SET(fileno(stdin), &readset);
	select(fileno(stdin)+1, &readset, NULL, NULL, &tv);
	// the user typed a character so exit
	if(FD_ISSET(fileno(stdin), &readset))
	{
		char input = fgetc(stdin); // read character
		if (pause_) engine_->pause(pause_=false);
		
		switch(input) {
				
			case 'q':
				engine_->stop();
				break;
			case 'h':
				printHelp();
				break;
			case 'v':
				verbose_=!verbose_;
				break;
			case 'p':
				pause_ = true;
				std::cout << "paused - press any key to continue" << std::endl;
				engine_->pause(true);
				break;
		}
	}

}

void ConsoleInterface::setBuffers(unsigned char *src, unsigned char *dest, int width, int height, bool color)
{
    width_  = width;
    height_ = height;

    sourceBuffer_ = src;
    destBuffer_   = dest;
}

void ConsoleInterface::printFrameRate() {
	
    frames_++;
    long currentTime_ = VisionEngine::currentSeconds();
    long diffTime = currentTime_ - lastTime_;
    
    if (diffTime >= 1) {
        current_fps_ = (int)floor( (frames_ / diffTime) + 0.5 );
		std::cout << current_fps_ << " FPS" << std::endl;
        lastTime_ = currentTime_;
        frames_ = 0;
    }
}

void ConsoleInterface::printMessage(std::string message)
{
    if (verbose_) {
        std::cout << message << std::endl;
    }
}

void ConsoleInterface::displayError(const char* error)
{
	std::cout <<  error << std::endl;
	engine_->stop();
}

void ConsoleInterface::printHelp()
{
	std::cout << std::endl;
    for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++) {
		std::cout << *help_line << std::endl;
    }
}

void ConsoleInterface::displayMessage(const char *message)
{
   std::cout << message << std::endl;
}

void ConsoleInterface::displayControl(const char *title, int min, int max, int value)
{

}

void ConsoleInterface::drawMark(int xpos, int ypos, const char *mark, int state) {

}

void ConsoleInterface::setDisplayMode(DisplayMode mode) {
    displayMode_ = mode;
}

void ConsoleInterface::setHelpText(std::vector<std::string> hlp) {
    
    for(std::vector<std::string>::iterator help_line = hlp.begin(); help_line!=hlp.end(); help_line++) {
        help_text.push_back(*help_line);
    }
    
    //print the help message
    std::cout << std::endl;
    for(std::vector<std::string>::iterator help_line = help_text.begin(); help_line!=help_text.end(); help_line++) {
        std::cout << *help_line << std::endl;
    } std::cout << std::endl;
}

ConsoleInterface::ConsoleInterface(const char* name)
: pause_( false )
, frames_( 0 )
, current_fps_( 0 )
, width_( WIDTH )
, height_( HEIGHT )
{
    displayMode_ = NO_DISPLAY;
    app_name_ = std::string(name);
    
    lastTime_ = VisionEngine::currentSeconds();
    cameraTime_ = processingTime_ = totalTime_ = 0.0f;
	
    sourceBuffer_ = NULL;
    destBuffer_ = NULL;
	displayBuffer_ = NULL;
	
    help_text.push_back("commands:");
    help_text.push_back("   q - quit " + app_name_);
    help_text.push_back("   v - verbose output");
    help_text.push_back("   h - show help text");
    help_text.push_back("   p - pause processing");

}



