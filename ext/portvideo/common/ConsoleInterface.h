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


#ifndef CONSOLENTERFACE_H
#define CONSOLENTERFACE_H

#include "UserInterface.h"
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>

class ConsoleInterface: public UserInterface
{

public:
	ConsoleInterface(const char* name);
	~ConsoleInterface() {
		tcsetattr(STDIN_FILENO, TCSANOW, &term_cfg);
		if (displayBuffer_) delete[] displayBuffer_;
	};

	unsigned char* openDisplay(VisionEngine *engine);
    void updateDisplay();
    void closeDisplay();
    
    void setHelpText(std::vector<std::string> hlp);
    void setBuffers(unsigned char *src, unsigned char *dest, int width, int height, bool color);
    void processEvents();
    void setDisplayMode(DisplayMode mode);
    
    void printMessage(std::string message);
    void displayMessage(const char *message);
    void displayControl(const char *title, int min, int max, int value);
    void displayError(const char* error);
    void drawMark(int xpos, int ypos, const char *mark, int state);
    
	void setVsync(bool vsync) {};

protected:

	void printHelp();
    void printFrameRate();

private:
    
    unsigned char* sourceBuffer_;
    unsigned char* destBuffer_;
	unsigned char* displayBuffer_;
    
    bool pause_;
	
	termios term_cfg;
	fd_set readset;
	struct timeval tv;
	
	long frames_;
  	long lastTime_;
	unsigned int cameraTime_, processingTime_, totalTime_;
    unsigned int current_fps_;
    
	int width_;
	int height_;

	std::string app_name_;
	std::vector<std::string> help_text;
};

#endif
