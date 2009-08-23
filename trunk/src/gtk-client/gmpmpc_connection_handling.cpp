#include "gmpmpc_connection_handling.h"
#include "../error-handling.h"
#include <boost/bind.hpp>
#include <gtkmm/main.h>
#include <string>
#include <map>

//  ????
// template<typename T1, typename T2>
// class sigcbooster {
// 	public:
// 	sigcbooster(boost::function<T1(T2)> t1) {
// 		f = t1;
// 	}
// 	void operator()(T2 x) {
// 		return f(x);
// 	}
// 	private:
// 		boost::function<T1(T2)> f;
// 		T2 arg_1;
// };

gmpmpc_connection_handler::gmpmpc_connection_handler() {
}

void gmpmpc_connection_handler::handle_message(const messageref m) {
	std::map<message::message_types, std::string> message_names;
	message_names[message::MSG_CONNECT]              = "MSG_CONNECT";
	message_names[message::MSG_ACCEPT]               = "MSG_ACCEPT";
	message_names[message::MSG_DISCONNECT]           = "MSG_DISCONNECT";
	message_names[message::MSG_PLAYLIST_UPDATE]      = "MSG_PLAYLIST_UPDATE";
	message_names[message::MSG_QUERY_TRACKDB]        = "MSG_QUERY_TRACKDB";
	message_names[message::MSG_QUERY_TRACKDB_RESULT] = "MSG_QUERY_TRACKDB_RESULT";
	message_names[message::MSG_REQUEST_FILE]         = "MSG_REQUEST_FILE";
	message_names[message::MSG_REQUEST_FILE_RESULT]  = "MSG_REQUEST_FILE_RESULT";
	message_names[message::MSG_VOTE]                 = "MSG_VOTE";
	message::message_types message_type = (message::message_types)m->get_type();
	if(message_names.find(message_type) != message_names.end())
		dcerr("Received a " << message_names[message_type]);

	switch(message_type) {
		case message::MSG_CONNECT: { } break;
		case message::MSG_ACCEPT: {
			connection_accepted_signal(boost::static_pointer_cast<message_accept>(m)->cid);
		}; break;
		case message::MSG_DISCONNECT: {
// 			gmpmpc_network_handler->client_message_receive_signal.disconnect(handle_received_message);
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
			playlist_update_signal(boost::static_pointer_cast<message_playlist_update>(m));
		}; break;
		case message::MSG_QUERY_TRACKDB: {
/*			message_query_trackdb_ref qr = boost::static_pointer_cast<message_query_trackdb>(m);
			vector<Track> res;
			BOOST_FOREACH(LocalTrack t, trackDB.search(qr->search)) {
				Track tr(TrackID(gmpmpc_client_id, t.id), t.metadata);
				res.push_back(tr);
			}
			message_query_trackdb_result_ref result(new message_query_trackdb_result(qr->qid, res));
			gmpmpc_network_handler->send_server_message(result);*/
		}; break;
		case message::MSG_QUERY_TRACKDB_RESULT: {}; break;
		case message::MSG_REQUEST_FILE: {
			request_file_signal(boost::static_pointer_cast<message_request_file>(m));
		}; break;
		case message::MSG_REQUEST_FILE_RESULT: {
			request_file_result_signal(boost::static_pointer_cast<message_request_file_result>(m));
		}; break;
		default: {
			dcerr("Ignoring unknown message of type " << m->get_type());
		} break;
	}
}
