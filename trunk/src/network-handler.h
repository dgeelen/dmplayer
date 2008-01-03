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
	#include "types.h"
	
	class network_handler {
		public:
			network_handler(uint16 port_number);
		private:
			tcp_listen_socket listen_sock;
			udp_socket udp_sock;
	};
#endif