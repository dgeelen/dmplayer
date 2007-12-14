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

tcp_listen_socket::tcp_listen_socket()
{
	sock = INVALID_SOCKET;
}

udp_socket::udp_socket()
{
	sock = INVALID_SOCKET;
}

tcp_listen_socket::tcp_listen_socket(const ipv4_addr addr, const uint16 portnumber)
{
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

udp_socket::udp_socket(const ipv4_addr addr, const uint16 portnumber)
{
	sock = socket( AF_INET, SOCK_DGRAM, 0 );

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = (addr.full == 0) ? INADDR_ANY : addr.full;
	addr_in.sin_port = htons( portnumber );

	if ( ::bind( sock, ( sockaddr* )&addr_in, sizeof( addr_in ) ) == SOCKET_ERROR ) {
		cout << "udp_socket: Bind to network failed:" << NetGetLastError() << "\n";
		closesocket( sock );
		throw("udp_socket: Bind to network failed!");
	}
	
	// now enable broadcasting
	unsigned long bc = 1;
	if ( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, ( char* )&bc, sizeof( bc )) == SOCKET_ERROR ) {
		cout << "udp_socket: Unable to enable broadcasting:" << NetGetLastError() << "\n";
		closesocket( sock );
		throw("udp_socket: Unable to enable broadcasting!");
	};
}

uint32 udp_socket::send( const ipv4_addr dest_addr, const uint16 dest_port, const uint8* buf, const uint32 len ) {
	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = (dest_addr.full == 0xffffffff) ? INADDR_BROADCAST : dest_addr.full; //TODO: Broadcast == 0xffffffff?
	addr_in.sin_port = htons( dest_port );
	return sendto(sock, buf, len, 0, (sockaddr*)&addr_in, sizeof(addr_in));
}

uint32 udp_socket::receive( ipv4_addr* from_addr, uint16* from_port, const uint8* buf, const uint32 len ) {
	sockaddr_in addr_in;
/*	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = INADDR_ANY; //(from_addr.full == 0xffffffff) ? INADDR_BROADCAST : from_addr.full;
	addr_in.sin_port = 0;//htons( from_port );*/
	socklen_t addr_in_len = sizeof(addr_in);
	uint32 retval = recvfrom( sock, (void*)buf, len, 0, (sockaddr*)&addr_in, &addr_in_len);
	from_addr->full = addr_in.sin_addr.s_addr;
	*from_port = ntohs(addr_in.sin_port);
}
