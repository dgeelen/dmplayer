//
// C++ Implementation: network-core
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "network-core.h"
#include <iostream>

using namespace std;
tcp_listen_socket::tcp_listen_socket(const ipv4_addr addr, const int portnumber) 
{
	
	#ifdef HAVE_WINSOCK
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		wVersionRequested = MAKEWORD( 2, 0 );
		err = WSAStartup( wVersionRequested, &wsaData );
		if (err!=0) 
			throw("error!");
	}
	#endif

	sock = socket( AF_INET, SOCK_STREAM, 0 );

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = (addr.full == 0) ? INADDR_ANY : addr.full;
	addr_in.sin_port = htons( portnumber );

	if ( bind( sock, ( sockaddr* )&addr_in, sizeof( addr_in ) ) == SOCKET_ERROR ) {
		cout << "tcp_listen_socket: Bind to network failed:" << NetGetLastError() << "\n";
		closesocket( sock );
		throw("tcp_listen_socket: Bind to network failed!");
	}
}
