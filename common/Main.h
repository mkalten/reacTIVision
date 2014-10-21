/*  reacTIVision tangible interaction framework
    Main.cpp
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

#ifndef REACTIVISION_H
#define REACTIVISION_H

struct reactivision_settings {
	char file[1024];
	int port;
	char host[1024];
	char tree_config[1024];
	char grid_config[1024];
	char midi_config[1024];
	char camera_config[1024];
	bool invert_x;
	bool invert_y;
	bool invert_a;
	bool midi;
	bool amoeba;
	bool classic;
	bool background;
    bool fullscreen;
    bool headless;
	int finger_size;
	int finger_sensitivity;	
	int gradient_gate;
    int tile_size;
    int thread_count;
	int display_mode;
};

#endif
