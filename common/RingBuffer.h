/*  portVideo, a cross platform camera framework
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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

class RingBuffer
{
public:
	RingBuffer(int size);
	~RingBuffer();
	
	int size();
	
	unsigned char* getNextBufferToWrite();
	void writeFinished();
	unsigned char* getNextBufferToRead();
	void readFinished();

private:
	int nextIndex( int index );
	int bufferSize;
	
	unsigned char* buffer[3];
	volatile char readIndex;
	volatile char writeIndex;
};

#endif
