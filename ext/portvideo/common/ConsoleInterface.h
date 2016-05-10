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


#ifndef CONSOLENTERFACE_H
#define CONSOLENTERFACE_H

#include "UserInterface.h"
#ifndef WIN32
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#endif

class ConsoleInterface: public UserInterface
{

public:
	ConsoleInterface(const char* name);
	~ConsoleInterface() {
#ifndef WIN32
		tcsetattr(STDIN_FILENO, TCSANOW, &term_cfg);
#endif
	};

	bool openDisplay(VisionEngine *engine);
    void updateDisplay();
    void closeDisplay();
    
    void setHelpText(std::vector<std::string> hlp);
    void setBuffers(unsigned char *src, unsigned char *dest, int width, int height, int format);
    void processEvents();
    void setDisplayMode(DisplayMode mode);
    
    void printMessage(std::string message);
    void displayMessage(const char *message);
    void displayControl(const char *title, int min, int max, int value);
    void displayError(const char* error);
	void drawText(int xpos, int ypos, const char *text);
	
	void setColor(unsigned char r, unsigned char g, unsigned char b) {};
	void drawPoint(int x,int y) {};
	void drawLine(int x1,int y1, int x2, int y2) {};
	void drawRect(int x,int y, int w, int h) {};
	void drawRect(int x,int y, int w, int h, float r) {};
	void fillRect(int x,int y, int w, int h) {};
	void drawEllipse(int x,int y, int w, int h) {};
	void drawEllipse(int x,int y, int w, int h, float r) {};
	void fillEllipse(int x,int y, int w, int h) {};

protected:

	void printHelp();
    void printFrameRate();

private:
        
    bool pause_;
#ifndef WIN32	
	termios term_cfg;
	fd_set readset;
	struct timeval tv;
#endif	
	long frames_;
  	long lastTime_;
	unsigned int cameraTime_, processingTime_, totalTime_;
    unsigned int current_fps_;

	std::string app_name_;
	std::vector<std::string> help_text;
};

#endif
