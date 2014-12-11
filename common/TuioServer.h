/*  reacTIVision tangible interaction framework
    TuioServer.h
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

#ifndef TUIOSERVER_H
#define TUIOSERVER_H

#include "osc/OscOutboundPacketStream.h"
#include "ip/NetworkingUtils.h"
#include "ip/UdpSocket.h"

#include "MessageServer.h"

#define IP_MTU_SIZE 1536
#define OBJ_MESSAGE_SIZE 108	// setMessage + seqMessage size
#define CUR_MESSAGE_SIZE 88

class TuioServer: public MessageServer {

  private:
    UdpTransmitSocket *transmitSocket;

    osc::OutboundPacketStream  *objPacket;
    osc::OutboundPacketStream  *curPacket;

    char *objBuffer; 
    char *curBuffer;

    int objMessages;
    int curMessages;
	
	char source[1024];
 
  public: 
    TuioServer(const char* address, int port);

    bool freeObjSpace();
    void addObjSeq(int fseq);
    void addObjSet(int s_id, int f_id, float x, float y, float a, float X, float Y, float A, float m, float r);
    void addObjAlive(int *id, int size);
    void sendObjMessages();

    bool freeCurSpace();
    void addCurSeq(int fseq);
    void addCurSet(int s_id, float x, float y, float X, float Y, float m);
    void addCurAlive(int *id, int size);
    void sendCurMessages();

    ~TuioServer();

    int getType() { return TUIO_SERVER; }
};
#endif
