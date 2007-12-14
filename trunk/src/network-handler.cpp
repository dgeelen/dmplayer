//
// C++ Implementation: network-protocol
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "network-handler.h"
#include "network-core.h"
#include <iostream>
using namespace std;

network_handler::network_handler(uint16 port_number) {
	#ifdef HAVE_WINSOCK //NOTE: This is done only once, network_handler is a singleton class
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 0 );
		if (WSAStartup( wVersionRequested, &wsaData ) != 0)
			throw("Error: could not initialize WINSOCK!");
	}
	#endif
	cout << "Initializing network_handler()\n";
	ipv4_addr listen_addr;
	cout << "Listening on "; for(int i=0; i<4; ++i) cout << (int)listen_addr.array[i] << "."; cout << ":" << port_number <<"\n";
	listen_sock = tcp_listen_socket(listen_addr, port_number);
	udp_sock = udp_socket( listen_addr, port_number );

	listen_addr.full = 0xffffffff;
	char buf[256] = "Halllo aaaaleemalaaaalalalaa!!!\n";
	char rbuf[256] = "                                 ";
	udp_sock.send( listen_addr, port_number, (uint8*)buf, 256 );
	udp_sock.receive( &listen_addr, &port_number, (uint8*)rbuf, 256 );
	cout << "From " << (int)listen_addr.array[0] << "."
	                << (int)listen_addr.array[1] << "."
	                << (int)listen_addr.array[2] << "."
	                << (int)listen_addr.array[3] << ":"
	                << port_number
	                << ":\n\""<< rbuf << "\"\n";
}
