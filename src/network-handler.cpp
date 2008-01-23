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
#include <boost/bind.hpp>

using namespace std;

/** Begin class network_handler **/
bool singleton = true;

network_handler::network_handler(uint16 tcp_port_number, string server_name) {
	this->server_name = server_name;
	server_mode = true;
	this->tcp_port_number = tcp_port_number;
	init();
}

network_handler::network_handler(uint16 tcp_port_number) {
	this->server_name = "NOT_A_SERVER";
	server_mode = false;
	this->tcp_port_number = tcp_port_number;
	init();
}

void network_handler::init() {
	are_we_done = false;
	dcerr("network_handler(): Initializing...");
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
	udp_qsock = udp_socket( broadcast_addr, 0);

	dcerr("Network send thread: starting ping loop");
	packet request_servers_packet;
	while(!are_we_done && receive_packet_handler_running) {
		dcerr("Requesting server list...");
		ping_cookie=rand();
		request_servers_packet.reset();
		request_servers_packet.serialize<uint8>(PT_QUERY_SERVERS);
		request_servers_packet.serialize<uint32>(ping_cookie);
		last_ping_time = get_time_us();
		udp_qsock.send(broadcast_addr, UDP_PORT_NUMBER, request_servers_packet);
		usleep(1000000); //TODO: Make this depend on the number of actual clients? (prevents network spam)

		vector<server_info> vsi;
		uint64 curtime = get_time_us();
		dcerr("---Evalutaing "<<known_servers.size()<<" servers...");
		for(map<ipv4_socket_addr, server_info>::iterator i = known_servers.begin(); i != known_servers.end(); ) {
			dcerr(i->second);
			dcerr("Curtime:" << curtime);
			dcerr("i->second.ping_last_seen" << i->second.ping_last_seen);
			dcerr("Diff: " << (curtime - i->second.ping_last_seen));
			if(((curtime - i->second.ping_last_seen) > (3000000))) {
				dcerr("Deleting server");
				known_servers.erase(i);
				i = known_servers.begin();
				continue;
			}
			else {
				vsi.push_back(i->second);
			}
			++i;
			dcerr("");
		}
		dcerr("---Eval_DONE");
		server_list_update_signal(vsi);
	}
}

void network_handler::receive_packet_handler() {
	uint16 rsock_port = UDP_PORT_NUMBER + (server_mode?0:1);
	uint16 ssock_port = 0;
	dcerr("Network receive thread: Opening sockets: receive socket=" << rsock_port << ", ssock_port="<<ssock_port<<" (0==random)");
	ipv4_addr listen_addr;
	uint16 port_number = 0;
// 	tcp_listen_socket tcp_sock = tcp_listen_socket(listen_addr, tcp_port_number);
	udp_rsock = udp_socket( listen_addr, rsock_port);
	udp_ssock = udp_socket( listen_addr, ssock_port);
	dcerr("Network IO thread: Listening on " << listen_addr << ":" << port_number);


	listen_addr.full = INADDR_BROADCAST;

	packet received_packet;
	uint32 message_length;

	while(!are_we_done && receive_packet_handler_running) {
		message_length = udp_rsock.receive( &listen_addr, &port_number, received_packet );
		if(message_length == SOCKET_ERROR)
			dcerr("network_handler: Network reports error #" << NetGetLastError());
		else {
			int packet_type = received_packet.deserialize<uint8>();
			switch(packet_type) {
				case PT_QUERY_SERVERS: {
					dcerr("Received a PT_QUERY_SERVERS from " << listen_addr << ":" << port_number );
					if(!server_mode) break;
					packet reply_packet;
					reply_packet.serialize<uint8>(PT_REPLY_SERVERS);
					reply_packet.serialize<uint16>(tcp_port_number);
					uint32 cookie; received_packet.deserialize(cookie);
					reply_packet.serialize<uint32>(cookie);
					reply_packet.serialize( (string)server_name );
					#ifdef DEBUG
						usleep(100000); //Fake some latency (0.1sec)
					#endif
					udp_ssock.send( listen_addr, UDP_PORT_NUMBER+1, reply_packet );
					break;
				}
				case PT_REPLY_SERVERS: {
					dcerr("Received a PT_REPLY_SERVERS from " << listen_addr << ":" << port_number );
					uint16 tcp_port; received_packet.deserialize(tcp_port);
					uint32 cookie; received_packet.deserialize(cookie);
					ipv4_socket_addr sa(listen_addr, tcp_port);
					struct server_info& si = known_servers[sa]; // Get/Add server to list of known servers

					si.sock_addr = sa;
					received_packet.deserialize(si.name);
					if (cookie == ping_cookie) {
						si.ping_last_seen = get_time_us();
						si.ping_micro_secs = si.ping_last_seen - last_ping_time;
					} else dcerr("COOKIE FAILURE! (expected 0x" << hex << ping_cookie << ", got 0x" << cookie << ")" << dec);
					break;
				}
				default:
					dcerr("Unknown packet type " << (int)packet_type);
			}
		}
	}
}

void network_handler::start() {
	if(thread_receive_packet_handler!=NULL || thread_send_packet_handler!=NULL) throw("Error: stop() the network_handler() before start()ing it!");
	dcerr("Starting network IO thread");
	receive_packet_handler_running = true;
	try {
		thread_receive_packet_handler = new boost::thread(ErrorHandler(boost::bind(&network_handler::receive_packet_handler, this)));
		if(!server_mode)
			thread_send_packet_handler = new boost::thread(ErrorHandler(boost::bind(&network_handler::send_packet_handler, this)));
	}
	catch(...) {
		receive_packet_handler_running = false;
		throw("Could not start one or more threads!");
	}
}

void network_handler::stop()
{
	dcerr("Stopping network IO thread");
	are_we_done = true; // signal the threads the end is near
	udp_qsock.close(); // kill the sockets to abort any blocking socket operations
	udp_rsock.close();
	udp_ssock.close();

	#ifndef __linux__
	/* There appears to be something funny going on under linux
	 * wrt closing blocking sockets, so until that is resolved
	 * we don't join on this threads.
	 */
	if (thread_receive_packet_handler) thread_receive_packet_handler->join();
	#endif
	if (thread_send_packet_handler) thread_send_packet_handler->join();
}
/** End class network_handler **/

network_handler::~network_handler()
{
	stop();
}

/** Begin ostream operators **/
ostream& operator<<(std::ostream& os, const server_info& si) {
	return os << "  Name     : '" << si.name << "'\n" <<
							 "  Address  : "  << si.sock_addr << "\n" <<
							 "  Last seen: "  << hex << "0x" << si.ping_last_seen << "\n" << dec <<
							 "  Ping     : "  << (si.ping_micro_secs / 1000000) << "." <<
																	 ((si.ping_micro_secs - 1000000*(si.ping_micro_secs/1000000))
																	  /(1000000/100)) << "s";
	}
/** End ostream operators **/
