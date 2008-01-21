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
#include "error-handling.h"
#include "network-core.h"
#include <iostream>
#include <sstream>

using namespace std;

tcp_listen_socket::tcp_listen_socket()
{
	sock = INVALID_SOCKET;
}

udp_socket::udp_socket()
{
	sock = INVALID_SOCKET;
}

tcp_socket::tcp_socket( const ipv4_addr addr, const uint16 port )
{
	sock = socket( AF_INET, SOCK_STREAM, 0 );

	if (sock == INVALID_SOCKET) throw "failed to create tcp socket";

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = addr.full;
	addr_in.sin_port = htons( port );

	int res = ::connect(sock, (sockaddr*)&addr_in, sizeof(addr_in));
	if (res == SOCKET_ERROR) throw "failed to bind tcp socket";
}

uint32 tcp_socket::send( const uint8* buf, const uint32 len )
{
	return ::send(sock, (char*)buf, len, 0);
}

uint32 tcp_socket::receive( const uint8* buf, const uint32 len )
{
	return ::recv(sock, (char*)buf, len, 0);
}

tcp_listen_socket::tcp_listen_socket(const ipv4_addr addr, const uint16 portnumber)
{
	sock = socket( AF_INET, SOCK_STREAM, 0 );

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = (addr.full == 0) ? INADDR_ANY : addr.full;
	addr_in.sin_port = htons( portnumber );

	if ( bind( sock, ( sockaddr* )&addr_in, sizeof( addr_in ) ) == SOCKET_ERROR ) {
		cout << "tcp_listen_socket: Bind to network failed: error " << NetGetLastError() << "\n";
		closesocket( sock );
		stringstream ss;
		ss << "tcp_listen_socket: Bind to network failed: error " << NetGetLastError();
		throw(ss.str().c_str());	}
}

udp_socket::udp_socket(const ipv4_addr addr, const uint16 portnumber) {
	sock = socket( AF_INET, SOCK_DGRAM, 0 );

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = (addr.full == 0) ? INADDR_ANY : addr.full;
	addr_in.sin_port = htons( portnumber );

	if (portnumber && ::bind( sock, ( sockaddr* )&addr_in, sizeof( addr_in ) ) == SOCKET_ERROR ) {
		dcerr("udp_socket: Bind to network failed: error " << NetGetLastError() << "\n");
		closesocket( sock );
		stringstream ss;
		ss << "udp_socket: Bind to network failed: error " << NetGetLastError();
		throw(ss.str().c_str());
	}

	// now enable broadcasting
	unsigned long bc = 1;
	if ( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, ( char* )&bc, sizeof( bc )) == SOCKET_ERROR ) {
		dcerr("udp_socket: Unable to enable broadcasting: error " << NetGetLastError() << "\n");
		closesocket( sock );
		stringstream ss;
		ss << "udp_socket: Unable to enable broadcasting: error " << NetGetLastError();
		throw(ss.str().c_str());
	};
}

uint32 udp_socket::send( const ipv4_addr dest_addr, const uint16 dest_port, const uint8* buf, const uint32 len ) {
	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = dest_addr.full;
	addr_in.sin_port = htons( dest_port );
	return sendto(sock, (char*)buf, len, 0, (sockaddr*) &addr_in, sizeof(addr_in));
}

uint32 udp_socket::receive( ipv4_addr* from_addr, uint16* from_port, const uint8* buf, const uint32 len ) {
	sockaddr_in addr_in;
	socklen_t addr_in_len = sizeof(addr_in);
	uint32 retval = recvfrom( sock, (char*)buf, len, 0, (sockaddr*)&addr_in, &addr_in_len);
	from_addr->full = addr_in.sin_addr.s_addr;
	*from_port = ntohs(addr_in.sin_port);
	return retval;
}

uint32 udp_socket::send( const ipv4_addr dest_addr, const uint16 dest_port, packet& p ) {
	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = dest_addr.full;
	addr_in.sin_port = htons( dest_port );
	return sendto(sock, (char*)p.data, p.data_length(), 0, (sockaddr*) &addr_in, sizeof(addr_in));
}

uint32 udp_socket::receive( ipv4_addr* from_addr, uint16* from_port, packet& p ) {
	p.reset();
	sockaddr_in addr_in;
	socklen_t addr_in_len = sizeof(addr_in);
	uint32 retval = recvfrom( sock, (char*)p.data, PACKET_SIZE, 0, (sockaddr*)&addr_in, &addr_in_len);
	from_addr->full = addr_in.sin_addr.s_addr;
	*from_port = ntohs(addr_in.sin_port);
	return retval;
}

void udp_socket::close()
{
	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

string ipv4_socket_addr::std_str() const {
	std::stringstream ss;
	ss << (*this);
	return ss.str();
};

/** Stream operators **/

std::ostream& operator<<(std::ostream& os, const ipv4_addr& addr) {
		return os << (int)addr.array[0] << "." << (int)addr.array[1] << "." << (int)addr.array[2] << "." << (int)addr.array[3];
}

std::ostream& operator<<(std::ostream& os, const ipv4_socket_addr& saddr) {
		return os << saddr.first << ":" << saddr.second;
}
