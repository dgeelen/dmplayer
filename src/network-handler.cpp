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

#include "cross-platform.h"
#include "network-handler.h"

using namespace std;

bool singleton = true;
network_handler::network_handler(uint16 port_number) {
	dcout("network_handler(): Initializing... ");
	this->port_number = port_number;
	thread = NULL;
	if(!singleton) throw("Error: network_handler() is a singleton class!");
	singleton=false;
	#ifdef HAVE_WINSOCK //NOTE: This is done only once, network_handler is a singleton class
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 0 );
		if (WSAStartup( wVersionRequested, &wsaData ) != 0)
			throw("Error: could not initialize WIN8SOCK!");
	}
	#endif
	dcout("done\n");
}

uint16 network_handler::get_port_number() {
	return port_number;
}

void network_handler::set_tcp_listen_socket(tcp_listen_socket lsock) {
	listen_sock = lsock;
}

void network_handler::set_udp_socket(udp_socket usock) {
	udp_sock = usock;
}

bool network_handler::is_running() {
	return this->thread_is_running;
}

// Declare this only in network_handler.cpp and before network_handler::start()
// to keep it hidden from outsiders.
static uint32 thread_function_count = 0; //FIXME: Needs locking
int WINAPI thread_function(network_handler* nh) {
	++thread_function_count;
	dcout("Network IO thread(" << thread_function_count << "): Opening sockets\n");
	ipv4_addr listen_addr;
	uint16 port_number = nh->get_port_number();
	tcp_listen_socket tcp_sock = tcp_listen_socket(listen_addr, port_number);
	udp_socket udp_sock = udp_socket( listen_addr, port_number);
	nh->set_tcp_listen_socket(tcp_sock);
	nh->set_udp_socket(udp_sock);
	dcout("Network IO thread(" << thread_function_count << "): Listening on " <<
	     (int)listen_addr.array[0] << "." <<
	     (int)listen_addr.array[1] << "." <<
	     (int)listen_addr.array[2] << "." <<
	     (int)listen_addr.array[3] << "." <<
	     ":" << port_number << "\n" );


	listen_addr.full = INADDR_BROADCAST;

	#define RECEIVE_BUFFER_SIZE 1400
	char rbuf[RECEIVE_BUFFER_SIZE];
	while(nh->is_running()) {
		udp_sock.receive( &listen_addr, &port_number, (uint8*)rbuf, 256 );
		#ifdef DEBUG
		dcout("Network IO thread(" << thread_function_count << "): Received a packet from "
				<< (int)listen_addr.array[0] << "."
				<< (int)listen_addr.array[1] << "."
				<< (int)listen_addr.array[2] << "."
				<< (int)listen_addr.array[3] << ":"
				<< port_number << "\n");
		#endif
	}
	--thread_function_count;
}

bool network_handler::start() {
	if(thread!=(THREAD)NULL) throw("Error: stop() the network_handler() before start()ing it!");
	dcout("Starting network IO thread\n");
	thread = spawnThread(thread_function, this);
	thread_is_running = thread != (THREAD)NULL;
	return thread_is_running;
}

bool network_handler::stop() {
	if(thread==(THREAD)NULL) return true;
	dcout("Stopping network IO thread\n");
	thread_is_running = false;
	//TODO: Wait for thread to finish
	return true;
}

