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
#include "packet.h"
#include "error-handling.h"
#include <unistd.h> //FIXME: Will be included in cross-platform.h
#include <boost/bind.hpp>

using namespace std;

/** Begin struct server_info **/
server_info::server_info(string name, uint64 ping) {
	this->name = name;
	this->ping_nano_secs = ping;
}

string server_info::get_name() {
	return name;
}

uint64 server_info::get_ping() {
	return ping_nano_secs;
}
/** End struct server_info **/

/** Begin class network_handler **/
bool singleton = true;
network_handler::network_handler(uint16 tcp_port_number) {
	dcerr("network_handler(): Initializing...");
	this->tcp_port_number = tcp_port_number;
	thread_send_packet_handler = NULL;
	thread_receive_packet_handler = NULL;
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
	start();
	dcerr("network_handler(): Initialization complete");
}

vector<server_info> network_handler::get_available_servers() {
	return know_servers;
}

uint16 network_handler::get_port_number() {
	return tcp_port_number;
}


void network_handler::find_available_servers() {

}

void network_handler::send_packet_handler() {
	dcerr("Network send thread: Opening sockets");
	ipv4_addr broadcast_addr;
	broadcast_addr.full = INADDR_BROADCAST;
	uint16 port_number = 0;
	udp_socket udp_ssock = udp_socket( broadcast_addr, 0);

	dcerr("Network send thread: starting ping loop");
	packet request_servers_packet;
	while(receive_packet_handler_running) {
		dcerr("Requesting server list...");
		request_servers_packet.curpos = 0;
		request_servers_packet.serialize<uint8>(PT_QUERY_SERVERS);
		request_servers_packet.serialize<uint32>(tcp_port_number);
		udp_ssock.send(broadcast_addr, UDP_PORT_NUMBER, request_servers_packet.data, request_servers_packet.curpos);
		sleep(1);
	}
}

void network_handler::receive_packet_handler() {
	dcerr("Network receive thread: Opening sockets");
	ipv4_addr listen_addr;
	uint16 port_number = 0;
	tcp_listen_socket tcp_sock = tcp_listen_socket(listen_addr, tcp_port_number);
	udp_socket udp_rsock = udp_socket( listen_addr, UDP_PORT_NUMBER);
	udp_socket udp_ssock = udp_socket( listen_addr, 0);
	dcerr("Network IO thread: Listening on " <<
	     (int)listen_addr.array[0] << "." <<
	     (int)listen_addr.array[1] << "." <<
	     (int)listen_addr.array[2] << "." <<
	     (int)listen_addr.array[3] << "." <<
	     ":" << port_number);


	listen_addr.full = INADDR_BROADCAST;

	packet received_packet;
	uint32 message_length;

	while(receive_packet_handler_running) {
		received_packet.curpos = 0; //FIXME: Should be done by sock_receive
		message_length = udp_rsock.receive( &listen_addr, &port_number, received_packet.data, PACKET_SIZE );
		if(message_length == SOCKET_ERROR)
			dcerr("network_handler: Network reports error #" << NetGetLastError());
		else {
				dcerr("Network IO thread: Received a packet from "
				<< (int)listen_addr.array[0] << "."
				<< (int)listen_addr.array[1] << "."
				<< (int)listen_addr.array[2] << "."
				<< (int)listen_addr.array[3] << ":"
				<< port_number );

			int packet_type = received_packet.deserialize<uint8>();
			switch(packet_type) {
				case PT_QUERY_SERVERS: {
					dcerr("Received a PT_QUERY_SERVER");
					packet reply_packet;
					reply_packet.curpos = 0;
					reply_packet.serialize<uint8>(PT_REPLY_SERVERS);
					reply_packet.serialize<uint16>(tcp_port_number);
					reply_packet.serialize<uint32>(received_packet.deserialize<uint32>());
					reply_packet.serialize( "test-server" );
					udp_ssock.send( listen_addr, UDP_PORT_NUMBER, reply_packet.data, reply_packet.curpos );
					break;
				}
				default:
					dcerr("Unknow packet type " << (int)packet_type);
			}
		}
	}
}

void network_handler::start() {
	if(thread_receive_packet_handler!=NULL || thread_send_packet_handler!=NULL) throw("Error: stop() the network_handler() before start()ing it!");
	dcerr("Starting network IO thread");
	receive_packet_handler_running = true;
	try {
		thread_receive_packet_handler = new boost::thread(error_handler(boost::bind(&network_handler::receive_packet_handler, this)));
		thread_receive_packet_handler = new boost::thread(error_handler(boost::bind(&network_handler::send_packet_handler, this)));
	}
	catch(...) {
		receive_packet_handler_running = false;
		throw("Could not start one or more threads!");
	}
}

void network_handler::stop() {
	if(thread_receive_packet_handler==NULL && thread_send_packet_handler==NULL) return;
	dcerr("Stopping network IO thread");
	//TODO: Wait for thread to finish
}
/** End class network_handler **/
