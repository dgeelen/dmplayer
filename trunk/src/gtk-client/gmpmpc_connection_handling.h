#ifndef GMPMPC_CONNECTION_HANDLING_H
	#define GMPMPC_CONNECTION_HANDLING_H
	#include "../packet.h"
	#include "boost/signal.hpp"

	class gmpmpc_connection_handler {
		public:
			gmpmpc_connection_handler();

			boost::signal<void(ClientID)> connection_accepted_signal;
			boost::signal<void(message_playlist_update_ref)> update_playlist_signal;
			boost::signal<void(message_request_file_ref)> request_file_signal;

			void handle_message(const messageref);
		private:
	};
#endif //GMPMPC_CONNECTION_HANDLING_H


// 	#include "gmpmpc.h"
// 	#include "../packet.h"
//
// 	void handle_received_message(const messageref m);

