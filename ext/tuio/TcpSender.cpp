/*
 TUIO C++ Library
 Copyright (c) 2005-2017 Martin Kaltenbrunner <martin@tuio.org>
 
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

#include "TcpSender.h"
using namespace TUIO;

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef WIN32
#define SHUT_RDWR SD_BOTH
#endif

typedef struct {
	TcpSender *sender;
#ifdef WIN32
	SOCKET socket;
#else
	int socket;
#endif
} TcpSenderClientData;

#ifdef  WIN32
static DWORD WINAPI ClientThreadFunc( LPVOID obj )
#else
static void* ClientThreadFunc( void* obj )
#endif
{
	TcpSenderClientData *client_data = static_cast<TcpSenderClientData*>(obj);
	TcpSender *sender = client_data->sender;
#ifdef WIN32
	SOCKET client = client_data->socket;
#else
	int client = client_data->socket;
#endif
	delete client_data;
	std::string type = sender->tuio_type();

	char buf[16];
	while (recv(client, buf, sizeof(buf),0)>0) {}

#ifdef WIN32
	WaitForSingleObject(sender->tcp_mutex,INFINITE);
	sender->tcp_client_list.remove(client);
	if (client==sender->tcp_socket) sender->tcp_socket = 0;
	if (sender->tcp_client_list.size()==0) sender->connected=false;
	ReleaseMutex(sender->tcp_mutex);
	closesocket(client);
#else
	pthread_mutex_lock(&sender->tcp_mutex);
	sender->tcp_client_list.remove(client);
	if (client==sender->tcp_socket) sender->tcp_socket = 0;
	if (sender->tcp_client_list.size()==0) sender->connected=false;
	pthread_mutex_unlock(&sender->tcp_mutex);
	close(client);
#endif
	std::cout << type << " connection closed"<< std::endl;

	return 0;
};

#ifdef  WIN32
static DWORD WINAPI ServerThreadFunc( LPVOID obj )
#else
static void* ServerThreadFunc( void* obj )
#endif
{
	TcpSender *sender = static_cast<TcpSender*>(obj);
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	
	std::cout << sender->tuio_type() << " socket created on port " << sender->port_no << std::endl;
	while (sender->tcp_socket) {
#ifdef WIN32
		SOCKET tcp_client = -1;
#else
		int tcp_client = -1;
#endif
		len = sizeof(client_addr);
		tcp_client = accept(sender->tcp_socket, (struct sockaddr*)&client_addr, &len);
#ifdef WIN32
		 //win32 workaround on exit
		if (!client_addr.sin_addr.S_un.S_addr && !client_addr.sin_port) return 0;
		if ((client_addr.sin_addr.S_un.S_addr==3435973836) && (client_addr.sin_port==52428)) return 0;
#endif

		if (tcp_client>0) { 
			std::cout << sender->tuio_type() << " client connected from " << inet_ntoa(client_addr.sin_addr) << "@" << client_addr.sin_port << std::endl;
			
#ifdef SO_NOSIGPIPE
			int optval = 1;
			setsockopt(tcp_client,SOL_SOCKET,SO_NOSIGPIPE, (const void *)&optval, sizeof(int));
#endif
			
#ifdef WIN32
			WaitForSingleObject(sender->tcp_mutex,INFINITE);
			sender->tcp_client_list.push_back(tcp_client);
			ReleaseMutex(sender->tcp_mutex);
#else
			pthread_mutex_lock(&sender->tcp_mutex);
			sender->tcp_client_list.push_back(tcp_client);
			pthread_mutex_unlock(&sender->tcp_mutex);
#endif
			sender->connected=true;
			sender->newClient(tcp_client);
			//std::cout << sender->tcp_client_list.size() << " clients connected"<< std::endl;	

			TcpSenderClientData *client_data = new TcpSenderClientData;
			client_data->sender = sender;
			client_data->socket = tcp_client;
			
#ifdef WIN32
			DWORD ClientThreadId;
			HANDLE client_thread = CreateThread( 0, 0, ClientThreadFunc, client_data, 0, &ClientThreadId );
			if (client_thread) CloseHandle(client_thread);
#else
			pthread_t client_thread;
			pthread_create(&client_thread , NULL, ClientThreadFunc,client_data);
			pthread_detach(client_thread);
#endif
		} else break;
	}
	
	return 0;
};

TcpSender::TcpSender()
	:connected (false)
{
	local = true;
	buffer_size = MAX_TCP_SIZE;
	port_no = 3333;
	server_thread = 0;
	
#ifndef WIN32
	pthread_mutex_init(&tcp_mutex,NULL);
#else
	tcp_mutex = CreateMutex(NULL,FALSE,NULL);
#endif
	
	tcp_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (tcp_socket < 0) {
		std::cerr << "could not create " << tuio_type() << " socket" << std::endl;
		return;
	}
	
#ifdef SO_NOSIGPIPE
	int optval = 1;
	setsockopt(tcp_socket,SOL_SOCKET,SO_NOSIGPIPE, (const void *)&optval, sizeof(int));
#endif
	
	struct sockaddr_in tcp_server;
	memset( &tcp_server, 0, sizeof (tcp_server));
	//unsigned long addr = inet_addr("127.0.0.1");
	//memcpy( (char *)&tcp_server.sin_addr, &addr, sizeof(addr));
	
	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = htons(3333);
	tcp_server.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	int ret = connect(tcp_socket,(struct sockaddr*)&tcp_server,sizeof(tcp_server));
	if (ret<0) {
#ifdef WIN32
		closesocket(tcp_socket);
#else
		close(tcp_socket);
#endif	
		tcp_socket = 0;
		std::cerr << "could not open " << tuio_type() << " connection to 127.0.0.1:3333" << std::endl;
		return;
	} else {
		std::cout << tuio_type() << " connection opened to 127.0.0.1:3333" << std::endl;
		tcp_client_list.push_back(tcp_socket);
		connected = true;
		
		TcpSenderClientData *client_data = new TcpSenderClientData;
		client_data->sender = this;
		client_data->socket = tcp_socket;
		
#ifdef WIN32
		server_thread = CreateThread( 0, 0, ClientThreadFunc, client_data, 0, &ServerThreadId );
#else
		pthread_create(&server_thread , NULL, ClientThreadFunc,client_data);
#endif

	}

}

TcpSender::TcpSender(const char *host, int port) 
	:connected (false)
{	
	if ((strcmp(host,"127.0.0.1")==0) || (strcmp(host,"localhost")==0)) {
		local = true;
	} else local = false;
	buffer_size = MAX_TCP_SIZE;
	port_no = port;
	server_thread = 0;
	
#ifndef WIN32
	pthread_mutex_init(&tcp_mutex,NULL);
#else
	tcp_mutex = CreateMutex(NULL,FALSE,NULL);
#endif
	
	tcp_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (tcp_socket < 0) {
		std::cerr << "could not create  " << tuio_type() << " socket" << std::endl;
		return;
	}
	
#ifdef SO_NOSIGPIPE
	int optval = 1;
	setsockopt(tcp_socket,SOL_SOCKET,SO_NOSIGPIPE, (const void *)&optval, sizeof(int));
#endif

	struct sockaddr_in tcp_server;
	memset( &tcp_server, 0, sizeof (tcp_server));
	unsigned long addr = inet_addr(host);
	if (addr != INADDR_NONE) {
		memcpy( (char *)&tcp_server.sin_addr, &addr, sizeof(addr));
	} else {
		struct hostent *host_info = gethostbyname(host);
		if (host_info == NULL) {
			std::cerr << "unknown host name: " << host << std::endl;
			throw std::exception();
		}
		memcpy( (char *)&tcp_server.sin_addr, host_info->h_addr, host_info->h_length );
	}

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = htons(port);

	int ret = connect(tcp_socket,(struct sockaddr*)&tcp_server,sizeof(tcp_server));
	if (ret<0) {
#ifdef WIN32
		closesocket(tcp_socket);
#else
		close(tcp_socket);
#endif	
		std::cerr << "could not open " << tuio_type() << " connection to " << host << ":"<< port << std::endl;
		throw std::exception();
	} else {
		std::cout << tuio_type() << " connection opened to " << host << ":"<< port << std::endl;
		tcp_client_list.push_back(tcp_socket);
		connected = true;
		
		TcpSenderClientData *client_data = new TcpSenderClientData;
		client_data->sender = this;
		client_data->socket = tcp_socket;
		
#ifdef WIN32
		server_thread = CreateThread( 0, 0, ClientThreadFunc, client_data, 0, &ServerThreadId );
#else
		pthread_create(&server_thread , NULL, ClientThreadFunc,client_data);
#endif

	}
}

TcpSender::TcpSender(int port)
	:connected (false)
{
	local = false;
	buffer_size = MAX_TCP_SIZE;
	port_no = port;
	server_thread = 0;
	
#ifndef WIN32
	pthread_mutex_init(&tcp_mutex,NULL);
#else
	tcp_mutex = CreateMutex(NULL,FALSE,NULL);
#endif
	
	tcp_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp_socket < 0) {
		std::cerr << "could not create TUIO/TCP socket" << std::endl;
		throw std::exception();
	}

	int optval = 1;
	#ifdef  WIN32
	int ret = setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEADDR, (const char *)&optval,  sizeof(int));
	#else
	int ret = setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEADDR, (const void *)&optval,  sizeof(int));
	#endif
	if (ret < 0) {
		std::cerr << "could not reuse TUIO/TCP socket address" << std::endl;
		throw std::exception();
	}
	
	struct sockaddr_in tcp_server;
	memset( &tcp_server, 0, sizeof (tcp_server));

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_addr.s_addr = htonl(INADDR_ANY);
	tcp_server.sin_port = htons(port);

	socklen_t len = sizeof(tcp_server);
	ret = bind(tcp_socket,(struct sockaddr*)&tcp_server,len);
	if (ret < 0) {
		std::cerr << "could not bind to TUIO/TCP socket on port " << port << std::endl;
		throw std::exception();
	}
	
	ret =  listen(tcp_socket, 1);
	if (ret < 0) {
		std::cerr << "could not start listening to TUIO/TCP socket" << std::endl;
#ifdef WIN32
		closesocket(tcp_socket);
#else
		close(tcp_socket);
#endif
		throw std::exception();
	}
				
#ifdef WIN32
	DWORD ServerThreadId;
	server_thread = CreateThread( 0, 0, ServerThreadFunc, this, 0, &ServerThreadId );
#else
	pthread_create(&server_thread , NULL, ServerThreadFunc, this);
#endif
	
}

bool TcpSender::isConnected() {
	return connected;
}


TcpSender::~TcpSender() {

	// shut down all sockets to unblock the sender threads
	// the client threads close their own sockets, so the main socket
	// is only closed here if it is not owned by a client thread
#ifdef WIN32
	bool close_socket = false;
	WaitForSingleObject(tcp_mutex,INFINITE);
	for (std::list<SOCKET>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++)
		shutdown((*client),SD_BOTH);
	if (tcp_socket>0) {
		close_socket = true;
		for (std::list<SOCKET>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++) {
			if ((*client)==tcp_socket) { close_socket = false; break; }
		}
		if (close_socket) shutdown(tcp_socket,SD_BOTH);
	}
	ReleaseMutex(tcp_mutex);
	if (close_socket) closesocket(tcp_socket);
#else
	bool close_socket = false;
	pthread_mutex_lock(&tcp_mutex);
	for (std::list<int>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++)
		shutdown((*client),SHUT_RDWR);
	if (tcp_socket>0) {
		close_socket = true;
		for (std::list<int>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++) {
			if ((*client)==tcp_socket) { close_socket = false; break; }
		}
		if (close_socket) shutdown(tcp_socket,SHUT_RDWR);
	}
	pthread_mutex_unlock(&tcp_mutex);
	if (close_socket) close(tcp_socket);
#endif
	tcp_socket = 0;

	// the client threads remove their sockets from the client list when they terminate
	if (server_thread) {
#ifndef WIN32
		pthread_join(server_thread,NULL);
#else
		WaitForSingleObject(server_thread,INFINITE);
		CloseHandle(server_thread);
#endif
		server_thread = 0;
	}
	
	// wait for the client threads to terminate
	bool clients_left = true;
	while (clients_left) {
#ifdef WIN32
		WaitForSingleObject(tcp_mutex,INFINITE);
		clients_left = (tcp_client_list.size()>0);
		ReleaseMutex(tcp_mutex);
		if (clients_left) Sleep(1);
#else
		pthread_mutex_lock(&tcp_mutex);
		clients_left = (tcp_client_list.size()>0);
		pthread_mutex_unlock(&tcp_mutex);
		if (clients_left) usleep(1000);
#endif
	}
	
#ifdef WIN32
	if (tcp_mutex) CloseHandle(tcp_mutex);
#else
	pthread_mutex_destroy(&tcp_mutex);
#endif
}


bool TcpSender::sendOscPacket (osc::OutboundPacketStream *bundle) {
	if (!connected) return false; 
	if ( bundle->Size() > buffer_size ) return false;
	if ( bundle->Size() == 0 ) return false;
	
#ifdef OSC_HOST_LITTLE_ENDIAN             
	data_size[0] =  bundle->Size()>>24;
	data_size[1] = (bundle->Size()>>16) & 255;
	data_size[2] = (bundle->Size()>>8) & 255;
	data_size[3] = (bundle->Size()) & 255;
#else
	*((int32_t*)data_size) = bundle->Size();
#endif

#ifdef WIN32
	std::list<SOCKET>::iterator client;
	WaitForSingleObject(tcp_mutex,INFINITE);
#else
	std::list<int>::iterator client;
	pthread_mutex_lock(&tcp_mutex);
#endif
	
	memcpy(&data_buffer[0], &data_size, 4);
	memcpy(&data_buffer[4], bundle->Data(), bundle->Size());
	
	for (client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++) {
		
		// keep sending until the complete packet has been written
		int packet_size = 4+bundle->Size();
		int sent_bytes = 0;
		while (sent_bytes<packet_size) {
			int bytes = send((*client),data_buffer+sent_bytes,packet_size-sent_bytes,MSG_NOSIGNAL);
			if (bytes<=0) {
				// shut down the socket so that the client thread removes it
				shutdown((*client),SHUT_RDWR);
				break;
			}
			sent_bytes += bytes;
		}
	}

#ifdef WIN32
	ReleaseMutex(tcp_mutex);
#else
	pthread_mutex_unlock(&tcp_mutex);
#endif

	return true;
}

void TcpSender::newClient( int tcp_client ) { }
