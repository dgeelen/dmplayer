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
#include <boost/foreach.hpp>

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
	thread_client_tcp_connection = NULL;
	thread_server_tcp_connection_listener = NULL;
	if(!singleton) throw LogicError("Error: network_handler() is a singleton class!");
	singleton=false;
	#ifdef HAVE_WINSOCK //NOTE: This is done only once, network_handler is a singleton class
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 0 );
		if (WSAStartup( wVersionRequested, &wsaData ) != 0)
			throw NetworkException("Error: could not initialize WIN8SOCK!");
	}
	#endif
	start();
	dcerr("network_handler(): Initialization complete");
}

uint16 network_handler::get_port_number() {
	return tcp_port_number;
}



void network_handler::server_tcp_connection_listener() { // Listens for incoming connections (Server)
	typedef pair<tcp_socket*, pair<boost::thread*, bool*> > pairtype;
	dcerr("");
	ipv4_addr addr;
	addr.full = INADDR_ANY;
	tcp_listen_socket lsock(addr, tcp_port_number);

	vector<pairtype> connections;
	while(!are_we_done) {
		tcp_socket* sock = lsock.accept();
		bool* b = new bool(true);
// 		boost::thread* t = new WRAP(server_tcp_connection_handler, this);
		boost::thread* t = new boost::thread(
		                       makeErrorHandler(
		                       boost::bind(&network_handler::server_tcp_connection_handler,
		                                   this,
		                                   sock,
		                                   b)));
		pair<boost::thread*, bool*> cc(t, b);
		pairtype c(sock, cc);
		connections.push_back(c);
	}
	BOOST_FOREACH(pairtype tpair, connections) {
	//for(vector<pair<tcp_socket*, pair<boost::thread*, bool*> > >::iterator i = connections.begin(); i!=connections.end(); ++i) {
		//TODO: disconnect
	}
}

void network_handler::client_tcp_connection(ipv4_socket_addr dest) { // Initiates a connection to the server (Client)
	tcp_socket sock(dest.first, dest.second);

	messageref msg(new message_connect());

	sock << msg;

	// TODO: rest of function, protocol implementation
	while(!are_we_done && client_tcp_connection_running) {
		uint32 sockstat = doselect(sock, 1000, SELECT_READ|SELECT_ERROR);
		if (sockstat & SELECT_READ) {
			messageref m;
			sock >> m;
			switch (m->get_type()) {
				case message::MSG_CONNECT: {
					// TODO: ?maybe?
				}; break;
				case message::MSG_DISCONNECT: {
					client_tcp_connection_running = false;
				}; break;
				case message::MSG_ACCEPT: {
					dcerr("Accepted a client connection from " << sock.get_ipv4_socket_addr());
					message_receive_signal(m);
				}; break;

			}
		}
		if (sockstat & SELECT_ERROR) {
			// TODO: error handling
//			*active = false;
		}
	}
}

void network_handler::server_tcp_connection_handler(tcp_socket* sock, bool* active) { // One thread per client (Server)
	while(!are_we_done && *active) {
		uint32 sockstat = doselect(*sock, 1000, SELECT_READ|SELECT_ERROR);
		if (sockstat & SELECT_READ) {
			messageref m;
			(*sock) >> m;
			switch (m->get_type()) {
				case message::MSG_CONNECT: {
					message_connect_ref msg = boost::static_pointer_cast<message_connect>(m);
					if (msg->get_version() == NETWERK_PROTOCOL_VERSION) {
						(*sock) << messageref(new message_accept());
					} else {
						(*sock) << messageref(new message_disconnect());
						*active = false;
					}
				}; break;
			}
		}
		if (sockstat & SELECT_ERROR) {
			// TOTO: error handling
			*active = false;
		}
	}
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
		//dcerr("Requesting server list...");
		ping_cookie=rand();
		request_servers_packet.reset();
		request_servers_packet.serialize<uint8>(PT_QUERY_SERVERS);
		request_servers_packet.serialize<uint32>(ping_cookie);
		last_ping_time = get_time_us();
		udp_qsock.send(broadcast_addr, UDP_PORT_NUMBER, request_servers_packet);
		usleep(1000000); //TODO: Make this depend on the number of actual clients? (prevents network spam)

		vector<server_info> vsi;
		uint64 curtime = get_time_us();
		//dcerr("---Evalutaing "<<known_servers.size()<<" servers...");
		for(map<ipv4_socket_addr, server_info>::iterator i = known_servers.begin(); i != known_servers.end(); ) {
// 			dcerr(i->second);
// 			dcerr("Curtime:" << curtime);
// 			dcerr("i->second.ping_last_seen" << i->second.ping_last_seen);
// 			dcerr("Diff: " << (curtime - i->second.ping_last_seen));
			if(((curtime - i->second.ping_last_seen) > (3000000))) {
				dcerr("Deleting server");
				server_list_removed_signal(i->second);
				known_servers.erase(i);
				i = known_servers.begin();
				continue;
			}
			else {
				vsi.push_back(i->second);
			}
			++i;
// 			dcerr("");
		}
		//dcerr("---Eval_DONE");
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
// 					dcerr("Received a PT_QUERY_SERVERS from " << listen_addr << ":" << port_number );
					if(!server_mode) break;
					packet reply_packet;
					reply_packet.serialize<uint8>(PT_REPLY_SERVERS);
					reply_packet.serialize<uint16>(tcp_port_number);
					uint32 cookie; received_packet.deserialize(cookie);
					reply_packet.serialize<uint32>(cookie);
					reply_packet.serialize( (string)server_name );
					udp_ssock.send( listen_addr, UDP_PORT_NUMBER+1, reply_packet );
					break;
				}
				case PT_REPLY_SERVERS: {
// 					dcerr("Received a PT_REPLY_SERVERS from " << listen_addr << ":" << port_number );
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
	dcerr("Starting network IO threads");
	if(thread_receive_packet_handler!=NULL || thread_send_packet_handler!=NULL)
		throw("Error: stop() the network_handler() before start()ing it!");
	receive_packet_handler_running = true;
	try {
		thread_receive_packet_handler = new boost::thread(makeErrorHandler(boost::bind(&network_handler::receive_packet_handler, this)));
		if(server_mode)
			thread_server_tcp_connection_listener = new boost::thread(makeErrorHandler(boost::bind(&network_handler::server_tcp_connection_listener, this)));
		if(!server_mode) {
			thread_send_packet_handler   = new boost::thread(makeErrorHandler(boost::bind(&network_handler::send_packet_handler, this)));
		}
	}
	catch(...) {
		receive_packet_handler_running = false;
		throw ThreadException("Could not start one or more threads!");
	}
}

void network_handler::client_connect_to_server( ipv4_socket_addr dest ) {
	dcerr(dest);
	client_tcp_connection_running = true;
	thread_client_tcp_connection = new boost::thread(makeErrorHandler(boost::bind(&network_handler::client_tcp_connection, this, dest)));
}

void network_handler::client_disconnect_from_server(  ) {
	client_tcp_connection_running = false;
	target_server = ipv4_socket_addr();
	if (thread_client_tcp_connection) {
		thread_client_tcp_connection->join();
		delete thread_client_tcp_connection;
		thread_client_tcp_connection=NULL;
	}
}
#undef WRAP

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
	if (thread_receive_packet_handler) {
		thread_receive_packet_handler->join();
		delete thread_receive_packet_handler;
		thread_receive_packet_handler=NULL;
	}
	#endif
	if (thread_send_packet_handler) {
		thread_send_packet_handler->join();
		delete thread_send_packet_handler;
		thread_receive_packet_handler=NULL;
	}
	if(thread_server_tcp_connection_listener) {
		thread_server_tcp_connection_listener->join();
		delete thread_server_tcp_connection_listener;
		thread_server_tcp_connection_listener=NULL;
	}
	client_disconnect_from_server();
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
