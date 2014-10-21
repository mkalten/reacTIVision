/*  portVideo, a cross platform camera framework
	MessageServer.h
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

#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H


#define TUIO_SERVER 0
#define MIDI_SERVER 1
#define TWO_PI 6.283185307179586232

class MessageServer
{
public:

	MessageServer() {
		invert_x = false;
		invert_y = false;
		invert_a = false;
		running = false;
	};	
	virtual ~MessageServer() {};
	
	virtual int getType() = 0;

	void setInversion(bool x, bool y, bool a) {
		invert_x = x;
		invert_y = y;
		invert_a = a;
	}

	void setInvertX(bool x) { invert_x = x; };
	void setInvertY(bool y) { invert_y = y; };
	void setInvertA(bool a) { invert_a = a; };
	
	bool getInvertX() { return invert_x; };
	bool getInvertY() { return invert_y; };
	bool getInvertA() { return invert_a; };
	bool isRunning() { return running; };

protected:
	bool invert_x;
	bool invert_y;
	bool invert_a;
	bool running;
};

#endif
