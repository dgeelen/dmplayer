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
#include <sstream>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/shared_array.hpp>

using namespace std;

#ifdef HAVE_WINSOCK
	WinSockClass winsockinstance;
	int WinSockClass::counter = 0;
#endif

tcp_socket::tcp_socket()
{
	sock = INVALID_SOCKET;
}

tcp_listen_socket::tcp_listen_socket()
{
	sock = INVALID_SOCKET;
}

udp_socket::udp_socket()
{
	sock = INVALID_SOCKET;
}

tcp_socket::tcp_socket( SOCKET s, ipv4_socket_addr addr) {
	sock = s;
	peer = addr;
}

tcp_socket::tcp_socket( const ipv4_addr addr, const uint16 port )
{
	this->connect(addr, port);
}

void tcp_socket::connect( const ipv4_addr addr, const uint16 port )
{
	this->disconnect();
	try {
		sock = socket( AF_INET, SOCK_STREAM, 0 );

		if (sock == INVALID_SOCKET) throw std::exception("failed to create tcp socket");

		sockaddr_in addr_in;
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.s_addr = addr.full;
		addr_in.sin_port = htons( port );

		int res = ::connect(sock, (sockaddr*)&addr_in, sizeof(addr_in));
		if (res == SOCKET_ERROR) throw std::exception("failed to connect tcp socket");

		peer = ipv4_socket_addr(ipv4_addr(addr_in.sin_addr.s_addr), addr_in.sin_port);
	} catch (...) {
		this->disconnect();
		throw;
	}
}

uint32 tcp_socket::send( const uint8* buf, const uint32 len )
{
	uint32 done = 0;
	while (done < len) {
		int sent = ::send(sock, (char*)buf+done, len-done, 0);
		if (sent <= 0)
			return done;
		done += sent;
	}
	return done;
}

uint32 tcp_socket::receive( const uint8* buf, const uint32 len )
{
	int read = ::recv(sock, (char*)buf, len, 0);
	if (read >= 0) return read;
	return 0;
}

void tcp_listen_socket::disconnect()
{
	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

void tcp_socket::disconnect()
{
	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

tcp_socket::~tcp_socket()
{
	disconnect();
}

tcp_listen_socket::~tcp_listen_socket()
{
	disconnect();
}

ipv4_socket_addr tcp_socket::get_ipv4_socket_addr() {
	return peer;
}

tcp_listen_socket::tcp_listen_socket(const uint16 portnumber)
{
	listen(ipv4_addr(0), portnumber);
}

tcp_listen_socket::tcp_listen_socket(const ipv4_addr addr, const uint16 portnumber)
{
	listen(addr, portnumber);
}

void tcp_listen_socket::listen(const ipv4_addr addr, const uint16 portnumber)
{
	disconnect();
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
		throw std::exception(ss.str().c_str());
	}

	if(::listen(sock, 16)) { //Queue up to 16 connections
		cout << "tcp_listen_socket: Could not listen() on socket: error " << NetGetLastError() << "\n";
	}
}

tcp_socket* tcp_listen_socket::accept() {
	sockaddr_in peer;
	socklen_t len = sizeof(peer);
	SOCKET s = ::accept(sock, ( sockaddr* )&peer, &len);
	if(s != SOCKET_ERROR)
		return new tcp_socket(s, ipv4_socket_addr(peer.sin_addr.s_addr,peer.sin_port));
	return NULL;
}

udp_socket::udp_socket(const ipv4_addr addr, const uint16 portnumber) {
	sock = socket( AF_INET, SOCK_DGRAM, 0 );

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = (addr.full == 0) ? INADDR_ANY : addr.full;
	addr_in.sin_port = htons( portnumber );

	if (portnumber && ::bind( sock, ( sockaddr* )&addr_in, sizeof( addr_in ) ) == SOCKET_ERROR ) {
		closesocket( sock );
		stringstream ss;
		ss << "udp_socket: Bind to network failed: error " << NetGetLastError();
		throw std::exception(ss.str().c_str());
	}

	// now enable broadcasting
	unsigned long bc = 1;
	if ( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, ( char* )&bc, sizeof( bc )) == SOCKET_ERROR ) {
		closesocket( sock );
		stringstream ss;
		ss << "udp_socket: Unable to enable broadcasting: error " << NetGetLastError();
		throw std::exception(ss.str().c_str());
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

ipv4_addr ipv4_lookup(const std::string& host)
{
	ipv4_addr address;
	{
		unsigned long hostaddr = inet_addr( host.c_str() );
		if( hostaddr == INADDR_NONE )
		{
			hostent *hostentry = gethostbyname( host.c_str() );
			if(hostentry)
				hostaddr = *reinterpret_cast<unsigned long *>( hostentry->h_addr_list[0] );
		}
		address.full = hostaddr;
	}
	return address;
}

void tcp_socket::swap(tcp_socket* o)
{
	ipv4_socket_addr taddr = peer;
	SOCKET tsock = sock;
	peer = o->peer;
	sock = o->sock;
	o->peer = taddr;
	o->sock = tsock;
}