#include "middle_end.h"
#include "error-handling.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp> // FIXME: should this not be included by <boost/filesystem.hpp>

using namespace std;
using namespace boost;

middle_end::middle_end()
: networkhandler(TCP_PORT_NUMBER),
  client_id(-1)
{
	networkhandler.server_list_update_signal.connect(bind(&middle_end::handle_sig_server_list_update, this, _1));
	networkhandler.client_message_receive_signal.connect(bind(&middle_end::handle_received_message, this, _1));
	trackdb.add_directory("/home/dafox/sharedfolder/music/");
	networkhandler.start();
}

middle_end::~middle_end() {
	networkhandler.stop();
	threads.join_all();
}

void middle_end::connect_to_server(const ipv4_socket_addr address, int timeout) {
	mutex::scoped_lock lock(dest_server_mutex);
	networkhandler.client_disconnect_from_server();
	dest_server = address;
	networkhandler.client_connect_to_server(address);
}

void middle_end::cancel_connect_to_server(const ipv4_socket_addr address) {
	mutex::scoped_lock lock(dest_server_mutex);
	dest_server = ipv4_socket_addr(); // invalid address
}

std::vector<server_info> middle_end::get_known_servers() {
	mutex::scoped_lock lock(known_servers_mutex);
	return known_servers;
}

/**
 * Connected to the networkhandler's server_list_update_signal
 * @param server_list is a list of server_info (servers currently known to the networkhandler)
 * @note If new servers are detected or old servers are missing the appropriate signal is raised.
 * @see sig_servers_deleted()
 * @see sig_servers_added()
 */
void middle_end::handle_sig_server_list_update(const vector<server_info>& server_list) {
	mutex::scoped_lock lock(known_servers_mutex);
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

void middle_end::_search_tracks(SearchID search_id, const Track query) {
	mutex::scoped_lock lock(search_mutex); //FIXME: Searching should be possible concurrently
	std::vector<LocalTrack> local_result = trackdb.search(query);
	std::vector<Track> result; // FIXME: Needs a vector<pair<SearchID, vector<Track> > > or similar so we can hold network search results
	BOOST_FOREACH(LocalTrack lt, local_result) {
		result.push_back( Track(TrackID( client_id, lt.id), lt.metadata ) );
	}
	sig_search_tracks(search_id, result);
}

SearchID middle_end::search_tracks(const Track query) {
	{
	mutex::scoped_lock lock(search_id_mutex);
	++search_id;
	}
	try {
	threads.create_thread(bind(&middle_end::_search_tracks, this, search_id, query));
	} catch(thread_resource_error e) {
		// FIXME: Figure out if this is an error which we should be catching or not
		//        thread_group does not remove a thread object when the thread
		//        finishes execution, so this may just indicate an out-of-memory-error,
		//        in which case there may be no sense in waiting for memory to become
		//        available...
		return SearchID(-1);
	}
	return search_id;
}

void middle_end::mylist_append(Track track) {
	client_synced_playlist.add(track);

	boost::filesystem::ofstream f;

	//FIXME: If this is going to take a while (e.g. slow connection / lots of tracks)
	//       then we should do this in a separate threads rather than force the client
	//       to wait.
	while(messageref msg = client_synced_playlist.pop_msg()) {
		networkhandler.send_server_message(msg);
	}
}

void middle_end::handle_message_request_file(const message_request_file_ref request, shared_ptr<bool> done) {
	MetaDataMap md;
	Track q(request->id, md);
	vector<LocalTrack> s = trackdb.search(q);
	if(s.size() == 1) {
		if(filesystem::exists(s[0].filename)) {
			filesystem::ifstream f( s[0].filename, std::ios_base::in | std::ios_base::binary );
			uint32 n = 1024 * 32; // Well over 5sec of 44100KHz 2Channel 16Bit WAV
			std::vector<uint8> data;
			data.resize(1024*1024);
			while(!(*done)) {
				f.read((char*)&data[0], n);
				uint32 read = f.gcount();
				if(read) {
					std::vector<uint8> vdata;
					std::vector<uint8>::iterator from = data.begin();
					std::vector<uint8>::iterator to = from + read;
					vdata.insert(vdata.begin(), from, to);
					//FIXME: it is impossible to detect if the server is still listening (or even alive)
					message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
					networkhandler.send_server_message(msg);
					if(n<1024*1024) {
						n<<=1;
					}
					//FIXME: this does not take network latency into account (LAN APP!)
					usleep(n*10); // Ease up on CPU usage, starts with ~0.3 sec sleep and doubles every loop
				}
				else { //EOF or Error reading file
					dcerr("EOF or Error reading file");
					*done = true;
				}
			}
			dcerr("Transfer complete");
		}
		else { // Error opening file // FIXME: This is a hack!
			dcerr("Warning: could not open " << s[0].filename << ", Sending as-is...");
			std::vector<uint8> vdata;
			BOOST_FOREACH(char c, s[0].filename.string()) {
				vdata.push_back((uint8)c);
			}
			message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
			networkhandler.send_server_message(msg);
		}
	}
	// Send final empty message
	std::vector<uint8> vdata;
	message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
	networkhandler. send_server_message(msg);

	{
		mutex::scoped_lock lock(file_requests_mutex);
		typedef pair<pair<shared_ptr<bool>, thread::id>, LocalTrackID> file_requests_type;
		for(vector<file_requests_type>::iterator i = file_requests.begin(); i != file_requests.end(); ++i) {
			if((*i).first.second == this_thread::get_id()) {
				file_requests.erase(i);
				break; // There is only one thread with this ID, and we just invalidated i
			}
		}
	}
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
			const message_accept_ref msg = static_pointer_cast<message_accept>(m);
			client_id = msg->cid;
			sig_client_id_changed(client_id);
			mutex::scoped_lock lock(dest_server_mutex);
			if((dest_server == networkhandler.get_target_server_address()) && (dest_server != ipv4_socket_addr())) {
				sig_connect_to_server_success(dest_server, msg->cid);
			}
			else {
				networkhandler.client_disconnect_from_server();
			}

// 			std::map<string, string> metadata;
// 			metadata["FILENAME"] = "/home/dafox/shared folder/music/Onegai Teacher - snow angel.mp3";
// 			Track track(std::pair<ClientID, LocalTrackID>(msg->cid,LocalTrackID(0)), metadata);
// 			message_playlist_update_ref reply(new message_playlist_update(track));
// 			networkhandler.send_server_message(reply);

		}; break;
		case message::MSG_DISCONNECT: {
			if((dest_server == networkhandler.get_target_server_address()) && (dest_server != ipv4_socket_addr())) {
				const message_disconnect_ref msg = static_pointer_cast<message_disconnect>(m);
				client_id = ClientID(-1);
				sig_client_id_changed(client_id);
				sig_disconnected(msg->get_reason());
			}
// 			gmpmpc_network_handler->client_message_receive_signal.disconnect(handle_received_message);
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
			if(!sig_update_playlist.empty()) {
				IPlaylistRef playlist = sig_update_playlist();
				const message_playlist_update_ref msg = static_pointer_cast<message_playlist_update>(m);
				msg->apply(playlist);
			}
// 			playlist_update_signal(static_pointer_cast<message_playlist_update>(m));
		}; break;
		case message::MSG_QUERY_TRACKDB: {
/*			message_query_trackdb_ref qr = static_pointer_cast<message_query_trackdb>(m);
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
			message_request_file_ref msg = static_pointer_cast<message_request_file>(m);
			mutex::scoped_lock lock(file_requests_mutex);
			shared_ptr<bool> b = shared_ptr<bool>(new bool(false));
			thread::id tid = threads.create_thread(bind(&middle_end::handle_message_request_file, this, msg, b))->get_id();
			file_requests.push_back(pair<pair<shared_ptr<bool>, thread::id>, LocalTrackID>(pair<shared_ptr<bool>, thread::id>(b, tid), msg->id.second));
// 			request_file_signal(static_pointer_cast<message_request_file>(m));
		}; break;
		case message::MSG_REQUEST_FILE_RESULT: {
// 			request_file_result_signal();
		}; break;
		default: {
			dcerr("Ignoring unknown message of type " << m->get_type());
		} break;
	}
}
