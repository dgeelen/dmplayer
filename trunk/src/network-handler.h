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
	#include "network-core.h"
	#include "threads.h"
	#include "types.h"

	class network_handler {
		public:
			network_handler(uint16 port_number);
			bool start();
			bool stop();
			bool is_running();
			uint16 get_port_number();
			void set_tcp_listen_socket(tcp_listen_socket lsock);
			void set_udp_socket(udp_socket usock);
		private:
			THREAD thread;
			bool thread_is_running;
			uint16 port_number;
			tcp_listen_socket listen_sock;
			udp_socket udp_sock;
	};
#endif
