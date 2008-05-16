#include "gmpmpc_connection_handling.h"
#include "gmpmpc_select_server.h"
#include "../error-handling.h"

void handle_received_message(const messageref m) {
	switch(m->get_type()) {
		case message::MSG_CONNECT: {
			dcerr("Received a MSG_CONNECT");
		} break;
		case message::MSG_ACCEPT: {
			dcerr("Received a MSG_ACCEPT");
			select_server_accepted();
		}; break;
		case message::MSG_DISCONNECT: {
			dcerr("Received a MSG_DISCONNECT");
			gmpmpc_network_handler->message_receive_signal.disconnect(handle_received_message);
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
			dcerr("Received a MSG_PLAYLIST_UPDATE");
		}; break;
		case message::MSG_QUERY_TRACKDB: {
			dcerr("Received a MSG_QUERY_TRACKDB");
		}; break;
		case message::MSG_QUERY_TRACKDB_RESULT: {
			dcerr("Received a MSG_QUERY_TRACKDB_RESULT");
		}; break;
		case message::MSG_REQUEST_FILE: {
			dcerr("Received a MSG_REQUEST_FILE");
		}; break;
		case message::MSG_REQUEST_FILE_RESULT: {
			dcerr("Received a MSG_REQUEST_FILE_RESULT");
		}; break;
		default: {
			dcerr("Ignoring unknown message of type " << m->get_type());
		} break;
	}
}
