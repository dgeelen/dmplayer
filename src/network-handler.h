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

	#define UDP_PORT_NUMBER 55555

	struct server_info {
		public:
			server_info(std::string name, uint64 ping);
			std::string get_name();
			uint64      get_ping();
		private:
			std::string name;
			uint64      ping_nano_secs;
	};

	class network_handler {
		public:
			network_handler(uint16 tcp_port_number);
			void receive_packet_handler();
			void send_packet_handler();
			bool is_running();
			uint16 get_port_number();
			std::vector<struct server_info> get_available_servers();
			// Initiates a search for servers, use get_available_servers to retrieve a list
			void find_available_servers(); //Should probably be private
		private:
			boost::thread* thread_receive_packet_handler;
			boost::thread* thread_send_packet_handler;
			bool receive_packet_handler_running;
			bool send_packet_handler_running;
			uint16 tcp_port_number;
			std::vector<struct server_info> know_servers;
			void start();
			void stop();
		};
#endif
