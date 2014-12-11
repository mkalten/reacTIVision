/*  reacTIVision tangible interaction framework
    MidiServer.h
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

#ifndef MIDISERVER_H
#define MIDISERVER_H

#include <list>
#include <fstream>
#include <iostream>

#include "MessageServer.h"
#include "SDLinterface.h"

#include "portmidi.h"
#include "porttime.h"

#include "tinyxml.h"

#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define XPOS 0
#define YPOS 1
#define ANGLE 2
#define PRESENCE 3

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

class MidiServer: public MessageServer {

	private:

		struct mapping {
			int device;
			int f_id;
			int dimension;
			int channel;
			int control;
			float min;
			float max;
			float range;
			int value;
		};

		PmStream* midi_out;

		void readConfig(const char* cfg);
		std::list<mapping> mapping_list;
		int device;

	public:
		MidiServer(const char* cfg);
		void sendAddMessage(int f_id);
		void sendRemoveMessage(int f_id);
		void sendControlMessage(int f_id, float xpos, float ypos, float angle);
		void sendNullMessages();
		~MidiServer();

		int getType() { return MIDI_SERVER; }
		
		static void listDevices();
};
#endif
