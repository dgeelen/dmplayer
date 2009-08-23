#include "middle_end.h"
#include "error-handling.h"
#include <boost/bind.hpp>

using namespace std;

middle_end::middle_end() : networkhandler(TCP_PORT_NUMBER) {
	networkhandler.server_list_update_signal.connect(boost::bind(&middle_end::handle_sig_server_list_update, this, _1));
	networkhandler.client_message_receive_signal.connect(boost::bind(&middle_end::handle_received_message, this, _1));
}

void middle_end::start() {
	networkhandler.start();
}

void middle_end::connect_to_server(const ipv4_socket_addr address, int timeout) {
	networkhandler.client_disconnect_from_server();
	dest_server = address;
	networkhandler.client_connect_to_server(address);
}

/**
 * Connected to the networkhandler's server_list_update_signal
 * @param server_list is a list of server_info (servers currently known to the networkhandler)
 * @note If new servers are detected or old servers are missing the appropriate signal is raised.
 * @see sig_servers_deleted()
 * @see sig_servers_added()
 */
void middle_end::handle_sig_server_list_update(const vector<server_info>& server_list) {
	vector<server_info> deleted_servers;
	vector<server_info> added_servers;

	// Scan for servers present in new list but not in known list (added servers)
	BOOST_FOREACH(server_info si_new, server_list) {
		bool present = false;
		BOOST_FOREACH(server_info si_old, known_servers) {
			if(si_new == si_old) {
				present = true;
				break;
			}
		}
		if(!present) {
			added_servers.push_back(si_new);
			known_servers.push_back(si_new);
		}
	}

	// Scan for servers present in known list but not in new list (deleted servers)
	BOOST_FOREACH(server_info si_old, known_servers) {
		bool present = false;
		BOOST_FOREACH(server_info si_new, server_list) {
			if(si_new == si_old) {
				present = true;
				break;
			}
		}
		if(!present) {
			deleted_servers.push_back(si_old);
		}
	}

	// Remove deleted_servers from known server list
	BOOST_FOREACH(server_info td, deleted_servers) {
		for(std::vector<server_info>::iterator i = known_servers.begin(); i != known_servers.end(); ++i) {
			if(td == *i) {
				known_servers.erase(i);
				break;
			}
  	}
	}

	if(deleted_servers.size())
		sig_servers_removed(deleted_servers);
	if(added_servers.size())
		sig_servers_added(added_servers);
}

ClientID middle_end::get_client_id() {
	return client_id;
}

/**
 * Handles messages received from the network. As much work as possible should
 * be done by the middle_end, leaving only the presentation to the GUI.
 */
void middle_end::handle_received_message(const messageref m) {
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
			const message_accept_ref msg = boost::static_pointer_cast<message_accept>(m);
			sig_connect_to_server_success(dest_server, msg->cid);
		}; break;
		case message::MSG_DISCONNECT: {
// 			gmpmpc_network_handler->client_message_receive_signal.disconnect(handle_received_message);
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
// 			playlist_update_signal(boost::static_pointer_cast<message_playlist_update>(m));
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
// 			request_file_signal(boost::static_pointer_cast<message_request_file>(m));
		}; break;
		case message::MSG_REQUEST_FILE_RESULT: {
// 			request_file_result_signal(boost::static_pointer_cast<message_request_file_result>(m));
		}; break;
		default: {
			dcerr("Ignoring unknown message of type " << m->get_type());
		} break;
	}
}
