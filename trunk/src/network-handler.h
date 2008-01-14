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


	struct server_info {
		public:
			std::string name;
			clock_t      ping_last_seen;
			clock_t      ping_micro_secs;
			ipv4_socket_addr sock_addr;

			server_info() : ping_last_seen(0), ping_micro_secs(0) {};
	};

	std::ostream& operator<<(std::ostream& os, const server_info& si);

	class network_handler {
		public:
			network_handler(uint16 tcp_port_number);
			void receive_packet_handler();
			void send_packet_handler();
			bool is_running();
			uint16 get_port_number();
			boost::signal<void(const std::vector<server_info>&)> add_server_signal;
		private:
			boost::thread* thread_receive_packet_handler;
			boost::thread* thread_send_packet_handler;
			bool receive_packet_handler_running;
			bool send_packet_handler_running;
			uint16 tcp_port_number;
			std::map<ipv4_socket_addr, server_info> known_servers;
			void start();
			void stop();
			clock_t last_ping_time;
			uint32 ping_cookie;
			ipv6_socket_addr self;
		};


#endif
