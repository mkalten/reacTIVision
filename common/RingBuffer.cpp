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

#include "RingBuffer.h"

RingBuffer::RingBuffer(int size) {
	readIndex  = 0;
	writeIndex = 0;
	bufferSize = size;
	
	buffer[0] = new unsigned char[bufferSize];
	buffer[1] = new unsigned char[bufferSize];
	buffer[2] = new unsigned char[bufferSize];
}


RingBuffer::~RingBuffer() {

	delete [] buffer[0];
	delete [] buffer[1];
	delete [] buffer[2];
}

int RingBuffer::size() {
	return bufferSize;
}

int RingBuffer::nextIndex( int index ) {
	if( index >= 2 )
		return 0;
	else
		return index + 1;
}

unsigned char* RingBuffer::getNextBufferToWrite() {
	int nextWriteIndex = nextIndex( writeIndex );
	if( nextWriteIndex == readIndex ){
		return NULL;
	}else{
		return buffer[ nextWriteIndex ];
	}
}

void RingBuffer::writeFinished() {
	writeIndex = nextIndex( writeIndex );
}


unsigned char* RingBuffer::getNextBufferToRead() {
	if( readIndex == writeIndex ){
		return NULL;
	}else{
		int nextReadIndex = nextIndex( readIndex );
		return buffer[ nextReadIndex ];
	}
}

void RingBuffer::readFinished() {
	readIndex = nextIndex( readIndex );
}
