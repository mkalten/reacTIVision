/*
 TUIO2 C++ Library
 Copyright (c) 2009-2015 Martin Kaltenbrunner <martin@tuio.org>
 WebSockSender (c) 2015 Florian Echtler <floe@butterbrot.org>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3.0 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library.
*/

#ifndef INCLUDED_WEBSOCKSENDER_H
#define INCLUDED_WEBSOCKSENDER_H


/* All of these macros assume use on a 32-bit variable.
 Additionally, SWAP assumes we're little-endian.	*/
#define SWAP(a) ((((a) >> 24) & 0x000000ff) | (((a) >>	8) & 0x0000ff00) | \
(((a) <<	8) & 0x00ff0000) | (((a) << 24) & 0xff000000))
#define ROL(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROR(a, b) ROL((a), (32 - (b)))
#define SHA1_HASH_SIZE (160/8)

#include "TcpSender.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>

namespace TUIO {
	
	/**
	 * The WebSockSender implements the WebSocket transport method for OSC
	 *
	 * @author Florian Echtler
	 * @version 2.0.a0
	 */ 
	class LIBDECL WebSockSender : public TcpSender {
				
	public:

		/**
		 * The default constructor creates a WebSockSender that listens to the default HTTP-alt port 8080 on localhost
		 */
		WebSockSender();
		
		/**
		 * This constructor creates a WebSockSender that listens to the provided port
		 *
		 * @param  port	the listening WebSocket port number
		 */
		WebSockSender(int port);	
		
		/**
		 * The destructor closes the socket. 
		 */
		~WebSockSender();
		
		/**
		 * This method delivers the provided OSC data
		 *
		 * @param *bundle  the OSC stream to deliver
		 * @return true if the data was delivered successfully
		 */
		bool sendOscPacket (osc::OutboundPacketStream *bundle);

		/**
		 * This method is called whenever a new client connects
		 *
		 * @param tcp_client the socket handle of the new client
		 */
		void newClient( int tcp_client );
	
	private:
		
		void sha1( uint8_t digest[SHA1_HASH_SIZE], const uint8_t* inbuf, size_t length );
		std::string base64( uint8_t* buffer, size_t size );
	};
}
#endif /* INCLUDED_WEBSOCKSENDER_H */
