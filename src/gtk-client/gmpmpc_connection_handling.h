#ifndef GMPMPC_CONNECTION_HANDLING_H
	#define GMPMPC_CONNECTION_HANDLING_H
	#include "../packet.h"
	#include "boost/signal.hpp"

	class gmpmpc_connection_handler {
		public:
			gmpmpc_connection_handler();

			boost::signal<void()> connection_accepted_signal;

			void handle_message(const messageref);
			void send_message_update_playlist(Track& t);
	};
#endif //GMPMPC_CONNECTION_HANDLING_H


// 	#include "gmpmpc.h"
// 	#include "../packet.h"
//
// 	void handle_received_message(const messageref m);

