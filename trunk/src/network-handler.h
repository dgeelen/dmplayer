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
	#include "network-core.h"
	#include <boost/thread/thread.hpp>
	#include <boost/signal.hpp>
	#include <map>
	#include <ctime>
	#include <iostream>
	#define UDP_PORT_NUMBER 55555
	#define MPMP_CLIENT false
	#define MPMP_SERVER true


	struct server_info {
		public:
			std::string name;
			uint64      ping_last_seen;
			uint64      ping_micro_secs;
			ipv4_socket_addr sock_addr;

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
		private:
			bool server_mode;
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
		};


#endif
