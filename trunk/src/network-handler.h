//
// C++ Interface: network-protocol
//
// Description:
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef NETWORK_PROTOCOL_H
	#define NETWORK_PROTOCOL_H
	#include "cross-platform.h"
	#include "network/network-core.h"
	#include "packet.h"
	#include "playlist_management.h"
	#include <boost/thread/thread.hpp>
	#include <boost/signal.hpp>
	#include <map>
	#include <ctime>
	#include <iostream>
	#define UDP_PORT_NUMBER 55555
	#define TCP_PORT_NUMBER 4444
	#define MPMP_CLIENT false
	#define MPMP_SERVER true

	struct server_info {
		public:
			std::string name;
			uint64      ping_last_seen;
			uint64      ping_micro_secs;
			ipv4_socket_addr sock_addr;
			bool        connected; // todo

			server_info() : ping_last_seen(0), ping_micro_secs(0) {};
	};

	std::ostream& operator<<(std::ostream& os, const server_info& si);

	class network_handler {
		public:
			network_handler(uint16 tcp_port_number);
			network_handler(uint16 tcp_port_number, std::string server_name);
			~network_handler();
			void receive_packet_handler();
			void send_packet_handler();
			bool is_running();
			uint16 get_port_number();

			boost::signal<void(const std::vector<server_info>&)> server_list_update_signal;
			boost::signal<void(const server_info&)> server_list_added_signal; // fixme: actually call this
			boost::signal<void(const server_info&)> server_list_removed_signal;
			boost::signal<void(const messageref)> client_message_receive_signal;
			boost::signal<void(const messageref, ClientID)> server_message_receive_signal;
			void client_connect_to_server( ipv4_socket_addr dest );
			void send_message(ClientID id, messageref msg);
			void send_server_message(messageref msg);

		private:
			bool server_mode;
			boost::thread* thread_server_tcp_connection_listener;
			boost::thread* thread_client_tcp_connection;
			boost::thread* thread_receive_packet_handler;
			boost::thread* thread_send_packet_handler;
			bool receive_packet_handler_running;
			bool send_packet_handler_running;
			uint16 tcp_port_number;
			std::map<ipv4_socket_addr, server_info> known_servers;
			void init();
			void start();
			std::string server_name;
			void stop();
			uint64 last_ping_time;
			uint32 ping_cookie;
			volatile bool are_we_done;
			udp_socket udp_ssock;
			udp_socket udp_rsock;
			udp_socket udp_qsock;

			std::map<uint32, std::pair<ClientID, TrackID> > vote_queue;
			uint32 cqid;

			boost::weak_ptr<tcp_socket> serversockweakref;

			/* Client connection with server */
			void client_tcp_connection(tcp_socket_ref sock);
			void client_disconnect_from_server();
			bool client_tcp_connection_running;
			ipv4_socket_addr target_server;

			/* Server connection with client */
			void server_tcp_connection_handler(tcp_socket_ref sock);
			void server_tcp_connection_listener();
			ClientID next_client_id;
			std::map<ClientID, boost::shared_ptr<tcp_socket> > clients;
		};


#endif
