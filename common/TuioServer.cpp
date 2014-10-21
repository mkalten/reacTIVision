/*  reacTIVision tangible interaction framework
    TuioServer.cpp
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

#include "TuioServer.h"
#include <stdio.h>
#include <iostream>
#ifdef WIN32
#include <Winsock2.h>
#include <time.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#endif

TuioServer::TuioServer(const char* address, int port)
{
	//InitializeNetworking();
	long unsigned int ip = GetHostByName(address);
	transmitSocket = new UdpTransmitSocket(IpEndpointName(ip, port));
	
	
	if ((strcmp(address, "localhost") == 0) || (strcmp(address, "127.0.0.1") == 0))  sprintf(source, "reacTIVision");
	else { 
		
		char hostname[64];
		char *source_addr = NULL;
		struct hostent *hp = NULL;
		struct in_addr *addr = NULL;
		
		gethostname(hostname, 64);
		hp = gethostbyname(hostname);
		
		if (hp==NULL) {
			sprintf(hostname, "%s.local", hostname);
			hp = gethostbyname(hostname);
		}
		
		if (hp!=NULL) {
			/*for (int i = 0; hp->h_addr_list[i] != 0; ++i) {
				addr = (struct in_addr *)(hp->h_addr_list[i]);
				//std::cout << inet_ntoa(*addr) << std::endl;
			}*/
			addr = (struct in_addr *)(hp->h_addr_list[0]);
			source_addr = inet_ntoa(*addr);
		} else {
			//generate a random internet address
			srand ( (unsigned int)time(NULL) );
			int r = rand();
			addr = (struct in_addr*)&r;
			source_addr = inet_ntoa(*addr);
		}		
		sprintf(source, "reacTIVision@%s", source_addr); 
	}
	//printf("%s\n",source);
	
	objBuffer = new char[IP_MTU_SIZE];
	objPacket = new osc::OutboundPacketStream(objBuffer,IP_MTU_SIZE);
	(*objPacket) << osc::BeginBundleImmediate;
	(*objPacket) << osc::BeginMessage( "/tuio/2Dobj") << "source" << source << osc::EndMessage;

	curBuffer = new char[IP_MTU_SIZE];
	curPacket = new osc::OutboundPacketStream(curBuffer,IP_MTU_SIZE);
	(*curPacket) << osc::BeginBundleImmediate;
	(*curPacket) << osc::BeginMessage( "/tuio/2Dcur") << "source" << source << osc::EndMessage;

	objMessages=0;
	curMessages=0;
	running=true;
}

bool  TuioServer::freeObjSpace() {
	return (((*objPacket).Capacity()-(*objPacket).Size())>=OBJ_MESSAGE_SIZE);
}

void TuioServer::addObjSeq(int fseq) {
	(*objPacket) << osc::BeginMessage( "/tuio/2Dobj") << "fseq"
		     << fseq << osc::EndMessage;
}

void TuioServer::addObjSet(int s_id, int f_id, float x, float y, float a, float X, float Y, float A, float m, float r) {

	if(invert_x) x=1-x;
	if(invert_y) y=1-y;
	if(invert_a) a=TWO_PI-a;

	(*objPacket) << osc::BeginMessage( "/tuio/2Dobj") << "set" 
		     << (int)s_id << f_id << x << y << a << X << Y << A << m << r
	             << osc::EndMessage;
	objMessages++;
}

void TuioServer::addObjAlive(int *id, int size) {
	(*objPacket) << osc::BeginMessage( "/tuio/2Dobj") << "alive";
	for (int i=0;i<size;i++)
		   (*objPacket) << id[i];	
	(*objPacket) << osc::EndMessage;
	objMessages++;
}

void TuioServer::sendObjMessages() {
	if (objMessages>0) {
		(*objPacket) << osc::EndBundle;

		transmitSocket->Send( objPacket->Data(), objPacket->Size() );
		objPacket->Clear();
		(*objPacket) << osc::BeginBundleImmediate;
		(*objPacket) << osc::BeginMessage( "/tuio/2Dobj") << "source" << source << osc::EndMessage;

		objMessages = 0;
	}
}

bool  TuioServer::freeCurSpace() {
	return ((curPacket->Capacity()-curPacket->Size())>=CUR_MESSAGE_SIZE);
}

void TuioServer::addCurSeq(int fseq) {
	(*curPacket) << osc::BeginMessage( "/tuio/2Dcur") << "fseq"
		     << fseq << osc::EndMessage;
}

void TuioServer::addCurSet(int s_id, float x, float y, float X, float Y, float m) {

	if(invert_x) x=1-x;
	if(invert_y) y=1-y;

	(*curPacket) << osc::BeginMessage( "/tuio/2Dcur") << "set" 
		     << s_id << x << y << X << Y << m
	             << osc::EndMessage;
	curMessages++;
}

void TuioServer::addCurAlive(int *id, int size) {
	(*curPacket) << osc::BeginMessage( "/tuio/2Dcur") << "alive";
	for (int i=0;i<size;i++)
		   (*curPacket) << id[i];	
	(*curPacket) << osc::EndMessage;
	curMessages++;
}

void TuioServer::sendCurMessages() {
	if (curMessages>0) {
		(*curPacket) << osc::EndBundle;

		transmitSocket->Send( curPacket->Data(), curPacket->Size() );
		curPacket->Clear();
		(*curPacket) << osc::BeginBundleImmediate;
		(*curPacket) << osc::BeginMessage( "/tuio/2Dcur") << "source" << source << osc::EndMessage;

		curMessages = 0;
	}
}

TuioServer::~TuioServer()
{
	delete objPacket;
	delete curPacket;
	delete []objBuffer;
	delete []curBuffer;
	
	delete transmitSocket;
	running=false;
}
