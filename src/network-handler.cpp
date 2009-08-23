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
#include "util/StrFormat.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace std;

/** Begin class network_handler **/
bool singleton = true;

/*NOTE: Enabling the next line will make is possible to run
 *			more than one client per machine, making it trivially
 *			possible to spam the playlist, so be sure to leave
 *			this _OFF_ in any build not exclusively used by a dev!
 */
//#define ALLOW_MULTIPLE_CLIENTS

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
	#ifdef DEBUG
	next_client_id = 0; //Intentionally not initialized
	#endif
	dcerr("network_handler(): Initializing...");
	thread_send_packet_handler.reset();
	thread_receive_packet_handler.reset();
	thread_client_tcp_connection.reset();
	thread_server_tcp_connection_listener.reset();
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
	dcerr("network_handler(): Initialization complete");
}

uint16 network_handler::get_port_number() {
	return tcp_port_number;
}



void network_handler::server_tcp_connection_listener() { // Listens for incoming connections (Server)
	ipv4_addr addr;
	addr.full = INADDR_ANY;
	tcp_listen_socket lsock(addr, tcp_port_number);
	tcp_port_number = lsock.getPortNumber();
	dcerr("Listening on tcp port " << tcp_port_number);
	while(!are_we_done) {
		if (doselect(lsock, 1000, SELECT_READ)) {
			tcp_socket_ref sock(lsock.accept());
			dcerr("Starting new server_tcp_connection_handler_thread");
			boost::shared_ptr<boost::thread> t = boost::shared_ptr<boost::thread>(new boost::thread(
								   makeErrorHandler(
								   boost::bind(&network_handler::server_tcp_connection_handler,
											   this,
											   sock))));
			server_tcp_connection_handler_threads.push_back(t);
		}
	}
	dcerr("bye!");
}

void network_handler::client_tcp_connection(tcp_socket_ref sock) { // Initiates a connection to the server (Client)
	messageref msg(new message_connect());

	*sock << msg;

	// TODO: rest of function, protocol implementation
	while(!are_we_done && client_tcp_connection_running) {
		uint32 sockstat = doselect(*sock, 1000, SELECT_READ|SELECT_ERROR);
		if (sockstat & (SELECT_READ|SELECT_ERROR)) {
			messageref m;
			*sock >> m;
			switch (m->get_type()) {
				case message::MSG_CONNECT: {
					// TODO: ?maybe?
				}; break;
				case message::MSG_DISCONNECT: {
					client_tcp_connection_running = false;
					client_message_receive_signal(m);
				}; break;
//				case message::MSG_QUERY_TRACKDB:
//				case message::MSG_ACCEPT:
//				case message::MSG_PLAYLIST_UPDATE:
				default: {
					client_message_receive_signal(m);
				}; break;
				//case message::

			}
		}
	}
	sock.reset();
}

void network_handler::server_tcp_connection_handler(tcp_socket_ref sock) { // One thread per client (Server)
	ClientID cid(next_client_id++);
	bool active = true;
	while(!are_we_done && active) {
		uint32 sockstat = doselect(*sock, 1000, SELECT_READ|SELECT_ERROR);
		if (sockstat & (SELECT_READ|SELECT_ERROR)) {
			messageref m;
			(*sock) >> m;
			switch (m->get_type()) {
				case message::MSG_CONNECT: {
					message_connect_ref msg = boost::static_pointer_cast<message_connect>(m);
					if(msg->get_version() == NETWORK_PROTOCOL_VERSION && msg->get_boost_version() == BOOST_VERSION) {
						dcerr("Accepted a client connection from " << sock->get_ipv4_socket_addr() << " ClientID="<<cid);
						{
						boost::mutex::scoped_lock lock(clients_mutex);
						clients[cid] = sock;
						}
						(*sock) << messageref(new message_accept(cid));
						server_message_receive_signal(m, cid);
					}
					else {
						dcerr("Client tried to connect with wrong version");
						dcerr("Client protocol version " << msg->get_version() << ", expected " << NETWORK_PROTOCOL_VERSION<< ")");
						dcerr("Boost version " << msg->get_boost_version() << ", expected " << BOOST_VERSION << ")");
						(*sock) << messageref(new message_disconnect(STRFORMAT("Protocol mismatch, you tried to connect with protocol v%i and Boost version %i while this server expects protocol v%i and boost version %i.",msg->get_version(), msg->get_boost_version(), NETWORK_PROTOCOL_VERSION,BOOST_VERSION)));
						active = false;
					}
				}; break;
				case message::MSG_DISCONNECT: {
					active = false;
					server_message_receive_signal(m, cid);
				}; break;
				default:
					server_message_receive_signal(m, cid);
			}
		}
	}
	boost::mutex::scoped_lock lock(clients_mutex);
	clients.erase(cid);
}

void network_handler::send_message_allclients(messageref msg) {
	boost::mutex::scoped_lock lock(clients_mutex);
	typedef std::pair<ClientID, tcp_socket_ref> vtype;
	BOOST_FOREACH(vtype cr, clients) {
		// NOTE: send_message() is copy&pasted here because of the lock() on 'clients_mutex'.
		//       Using a recursive mutex would be more difficult because server_tcp_connection_handler()
		//       may change the 'clients' vector.
		map<ClientID, boost::shared_ptr<tcp_socket> >::iterator sock = clients.find(cr.first);
		if(sock != clients.end()) {
			(*(sock->second)) << msg;
		}
	}
}
void network_handler::send_message(ClientID id, messageref msg) {
	boost::mutex::scoped_lock lock(clients_mutex);
	map<ClientID, boost::shared_ptr<tcp_socket> >::iterator sock = clients.find(id);
	if(sock != clients.end()) {
		(*(sock->second)) << msg;
	}
}

void network_handler::send_server_message(messageref msg) {
	tcp_socket_ref servsock = serversockweakref.lock();
	if (!servsock) throw std::runtime_error("no valid server connected");
	*servsock << msg;
}

void network_handler::send_packet_handler() {
	dcerr("Network send thread: Opening sockets");
	ipv4_addr broadcast_addr;
	broadcast_addr.full = INADDR_BROADCAST;
	uint16 port_number = 0;
	udp_qsock = udp_socket(0, 0);

	dcerr("Network send thread: starting ping loop");
	packet request_servers_packet;
	while(!are_we_done && receive_packet_handler_running) {
		//dcerr("Requesting server list...");
		ping_cookie=rand();
		request_servers_packet.reset();
		request_servers_packet.serialize<uint8>(PT_QUERY_SERVERS);
		request_servers_packet.serialize<uint32>(ping_cookie);
		last_ping_time = get_time_us();
#ifdef ALLOW_MULTIPLE_CLIENTS
		udp_rsock.send(broadcast_addr, UDP_PORT_NUMBER, request_servers_packet);
#else
		udp_qsock.send(broadcast_addr, UDP_PORT_NUMBER, request_servers_packet);
#endif
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
#ifdef ALLOW_MULTIPLE_CLIENTS
	uint16 rsock_port = server_mode?UDP_PORT_NUMBER:0;
#else
	uint16 rsock_port = UDP_PORT_NUMBER + (server_mode?0:1);
#endif
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
			received_packet.reset();
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
#ifdef ALLOW_MULTIPLE_CLIENTS
					udp_ssock.send( listen_addr, port_number, reply_packet );
#else
					udp_ssock.send( listen_addr, UDP_PORT_NUMBER+1, reply_packet );
#endif
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
	try {
		dcerr("Starting thread_receive_packet_handler");
		receive_packet_handler_running = true;
		thread_receive_packet_handler = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(boost::bind(&network_handler::receive_packet_handler, this))));
		if(server_mode) {
			dcerr("Starting thread_server_tcp_connection_listener");
			thread_server_tcp_connection_listener = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(boost::bind(&network_handler::server_tcp_connection_listener, this))));
		}
		if(!server_mode) {
			dcerr("Starting thread_send_packet_handler");
			thread_send_packet_handler   = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(boost::bind(&network_handler::send_packet_handler, this))));
		}
	}
	catch(...) {
		receive_packet_handler_running = false;
		throw ThreadException("Could not start one or more threads!");
	}
}

void network_handler::client_connect_to_server( ipv4_socket_addr dest ) {
	dcerr(dest);
	client_disconnect_from_server();
	client_tcp_connection_running = true;
	tcp_socket_ref serversock(new tcp_socket(dest.first, dest.second));
	serversockweakref = serversock;
	dcerr("Starting thread_client_tcp_connection");
	thread_client_tcp_connection = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(boost::bind(&network_handler::client_tcp_connection, this, serversock))));
}

void network_handler::client_disconnect_from_server(  ) {
	client_tcp_connection_running = false;
	target_server = ipv4_socket_addr();
	if (thread_client_tcp_connection) {
		dcerr("Joining thread_client_tcp_connection");
		thread_client_tcp_connection->join();
		thread_client_tcp_connection.reset();
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

	if (thread_receive_packet_handler) {
		dcerr("Joining thread_receive_packet_handler");
		thread_receive_packet_handler->join();
		thread_receive_packet_handler.reset();
	}
	if (thread_send_packet_handler) {
		dcerr("Joining thread_send_packet_handler");
		thread_send_packet_handler->join();
		thread_receive_packet_handler.reset();
	}
	if(thread_server_tcp_connection_listener) {
		dcerr("Joining thread_server_tcp_connection_listener");
		thread_server_tcp_connection_listener->join();
		thread_server_tcp_connection_listener.reset();
	}
	client_disconnect_from_server();
	typedef boost::shared_ptr<boost::thread> vtype;
	dcerr("Joining server_tcp_connection_handler_threads");
	BOOST_FOREACH(vtype tp, server_tcp_connection_handler_threads) {
		tp->join();
	}
}
/** End class network_handler **/

network_handler::~network_handler()
{
	dcerr("shutting down");
	stop();
	dcerr("shut down");
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
