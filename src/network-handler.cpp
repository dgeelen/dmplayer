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

uint16 network_handler::get_port_number() {
	return tcp_port_number;
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
		ping_cookie=rand();
		request_servers_packet.reset();
		request_servers_packet.serialize<uint8>(PT_QUERY_SERVERS);
		request_servers_packet.serialize<uint32>(ping_cookie);
		last_ping_time = clock();
		udp_ssock.send(broadcast_addr, UDP_PORT_NUMBER, request_servers_packet);
		usleep(1000000); //TODO: Make this depend on the number of actual clients? (prevents network spam)

		dcerr("---\nList of known servers:");
		bool server_list_changed = false;
		for(map<ipv4_socket_addr, server_info>::iterator i = known_servers.begin(); i != known_servers.end(); ++i) {
			time_t curtime = clock();
			if(curtime - i->second.ping_last_seen > CLOCKS_PER_SEC * 3 ) {
				known_servers.erase(i);
				dcerr((i->second));
			}
		}
		dcerr("---");
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
		message_length = udp_rsock.receive( &listen_addr, &port_number, received_packet );
		if(message_length == SOCKET_ERROR)
			dcerr("network_handler: Network reports error #" << NetGetLastError());
		else {
			dcerr("Network IO thread: Received a packet from " << listen_addr << ":" << port_number );

			int packet_type = received_packet.deserialize<uint8>();
			switch(packet_type) {
				case PT_QUERY_SERVERS: {
					dcerr("Received a PT_QUERY_SERVERS");
					packet reply_packet;
					reply_packet.serialize<uint8>(PT_REPLY_SERVERS);
					reply_packet.serialize<uint16>(tcp_port_number);
					reply_packet.serialize<uint32>(received_packet.deserialize<uint32>());
					reply_packet.serialize<string>( "test-server" );
					#ifdef DEBUG
						usleep(100000); //Fake some latency (0.1sec)
					#endif
					udp_ssock.send( listen_addr, UDP_PORT_NUMBER, reply_packet );
					break;
				}
				case PT_REPLY_SERVERS: {
					dcerr("Received a PT_REPLY_SERVERS");
					uint16 tcp_port; received_packet.deserialize(tcp_port);
					uint32 cookie; received_packet.deserialize(cookie);
					ipv4_socket_addr sa(listen_addr, tcp_port);
					struct server_info& si = known_servers[sa]; // Get/Add server to list of known servers

					si.sock_addr = sa;
					received_packet.deserialize(si.name);
					if (cookie == ping_cookie) {
						si.ping_last_seen = clock();
						si.ping_micro_secs = si.ping_last_seen - last_ping_time;
					} else dcerr("COOKIE FAILURE! (expected 0x" << hex << ping_cookie << ", got 0x" << cookie << ")" << dec);
					dcerr(si);
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

/** Begin ostream operators **/
ostream& operator<<(std::ostream& os, const server_info& si) {
	return os << "  Name     : '" << si.name << "'\n" <<
							 "  Address  : "  << si.sock_addr << "\n" <<
							 "  Last seen: "  << hex << "0x" << si.ping_last_seen << "\n" << dec <<
							 "  Ping     : "  << (si.ping_micro_secs / CLOCKS_PER_SEC) << "." <<
																	 ((si.ping_micro_secs - CLOCKS_PER_SEC*(si.ping_micro_secs/CLOCKS_PER_SEC))
																	  /(CLOCKS_PER_SEC/100)) << "s";
	}
/** End ostream operators **/
