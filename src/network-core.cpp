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

#include <boost/serialization/shared_ptr.hpp>

using namespace std;

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
	sock = socket( AF_INET, SOCK_STREAM, 0 );

	if (sock == INVALID_SOCKET) throw NetworkException("failed to create tcp socket");

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = addr.full;
	addr_in.sin_port = htons( port );

	int res = ::connect(sock, (sockaddr*)&addr_in, sizeof(addr_in));
	if (res == SOCKET_ERROR) throw NetworkException("failed to connect tcp socket");

	peer = ipv4_socket_addr(ipv4_addr(addr_in.sin_addr.s_addr), addr_in.sin_port);
}

uint32 tcp_socket::send( const uint8* buf, const uint32 len )
{
	return ::send(sock, (char*)buf, len, 0);
}

uint32 tcp_socket::receive( const uint8* buf, const uint32 len )
{
	return ::recv(sock, (char*)buf, len, 0);
}

// void tcp_socket::operator<<(const message*& msg) {
// void tcp_socket::operator<<(const message* const & msg) {
void tcp_socket::operator<<(const messagecref msg) {
	stringstream ss;
	boost::archive::text_oarchive oa(ss);
// 	const message_connect msg2;
// 	const message* const msg1=&msg2;
	oa << msg;
	cout << "Sending on network:\n\""<<ss.str()<<"\"\n";
	long l = htonl((long)ss.str().size() + 1);
	send((const uint8*)(&l), sizeof(long));
	send((const uint8*)ss.str().c_str(), ss.str().size() +1);
}

void tcp_socket::operator>>(      messageref& msg) {
	try {
		long l;
		int rnum;
		rnum = receive((uint8*)&l, sizeof(long));
		if (rnum != sizeof(long))
			throw NetworkException("receive failed");
		l = ntohl(l);
		boost::shared_ptr<uint8> a = boost::shared_ptr<uint8>(new uint8[l]);
		rnum = receive(a.get(),l);
		if (rnum != l)
			throw NetworkException("receive failed");
		stringstream ss((char*)a.get());
		cout << "Received on network:\n\""<<ss.str()<<"\"\n";
		boost::archive::text_iarchive ia(ss);
		ia >> msg;
	} catch (NetworkException e) {
		msg = messageref(new message_disconnect);
	}
}

void tcp_socket::disconnect()
{
	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

ipv4_socket_addr tcp_socket::get_ipv4_socket_addr() {
	return peer;
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
		throw NetworkException(ss.str());
	}

	if(listen(sock, 16)) { //Queue up to 16 connections
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
		dcerr("udp_socket: Bind to network failed: error " << NetGetLastError() << "\n");
		closesocket( sock );
		stringstream ss;
		ss << "udp_socket: Bind to network failed: error " << NetGetLastError();
		throw NetworkException(ss.str());
	}

	// now enable broadcasting
	unsigned long bc = 1;
	if ( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, ( char* )&bc, sizeof( bc )) == SOCKET_ERROR ) {
		dcerr("udp_socket: Unable to enable broadcasting: error " << NetGetLastError() << "\n");
		closesocket( sock );
		stringstream ss;
		ss << "udp_socket: Unable to enable broadcasting: error " << NetGetLastError();
		throw NetworkException(ss.str());
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
