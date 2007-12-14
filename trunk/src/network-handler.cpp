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

#include "network-handler.h"
#include "network-core.h"
#include <iostream>
using namespace std;

network_handler::network_handler(uint16 port_number) {
	cout << "Initializing network_handler()\n";
	ipv4_addr listen_addr;
	cout << "Listening on "; for(int i=0; i<4; ++i) cout << (int)listen_addr.array[i] << "."; cout << ":" << port_number <<"\n";
	tcp_listen_socket listen_sock(listen_addr, port_number);
}
