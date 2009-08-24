#include "middle_end.h"
#include "error-handling.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp> // FIXME: should this not be included by <boost/filesystem.hpp> ?

using namespace std;
using namespace boost;

middle_end::middle_end()
: networkhandler(TCP_PORT_NUMBER),
  client_id(-1)
{
	networkhandler.server_list_update_signal.connect(bind(&middle_end::handle_sig_server_list_update, this, _1));
	networkhandler.client_message_receive_signal.connect(bind(&middle_end::handle_received_message, this, _1));
	trackdb.add_directory("/home/dafox/sharedfolder/music/");
	trackdb.add_directory("f:\\home\\dafox\\sharedfolder\\music\\");
	trackdb.add_directory("d:\\music\\");
	networkhandler.start();
}

void middle_end::abort_all_file_transfers() {
	bool done(false);
	while(!done) {
		mutex::scoped_lock lock(file_requests_mutex);
		vector<file_requests_type>::iterator i = file_requests.begin();
		if(i != file_requests.end()) {
			lock.unlock();
			abort_file_transfer((*i).second);
		}
		else {
			done = true;
		}
	}
}

middle_end::~middle_end() {
	abort_all_file_transfers();
	networkhandler.stop();
	threads.join_all();
}

void middle_end::connect_to_server(const ipv4_socket_addr address, int timeout) {
	{
		mutex::scoped_lock lock(dest_server_mutex);
		dest_server = ipv4_socket_addr(); // invalid address
	}
	networkhandler.client_disconnect_from_server();
	{
		mutex::scoped_lock lock(client_id_mutex);
		client_id = ClientID(-1);
		sig_client_id_changed(client_id);
	}
	{
		mutex::scoped_lock lock(dest_server_mutex);
		dest_server = address;
	}
	networkhandler.client_connect_to_server(address);
}

void middle_end::cancel_connect_to_server(const ipv4_socket_addr address) {
	mutex::scoped_lock lock(client_id_mutex);
	if(client_id == ClientID(-1)) { // Not connected
		mutex::scoped_lock lock(dest_server_mutex);
		dest_server = ipv4_socket_addr(); // invalid address
	}
}

void middle_end::refresh_server_list() {
	mutex::scoped_lock lock(known_servers_mutex);
	sig_servers_removed(known_servers);
	known_servers.clear();
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

	if(deleted_servers.size()) {
		sig_servers_removed(deleted_servers);
	}
	if(added_servers.size()) {
		sig_servers_added(added_servers);
	}
}

ClientID middle_end::get_client_id() {
	mutex::scoped_lock lock(client_id_mutex);
	return client_id;
}

void middle_end::_search_tracks(SearchID search_id, const Track query) {
	mutex::scoped_lock lock(search_mutex); //FIXME: Searching should be possible concurrently
	std::vector<LocalTrack> local_result = trackdb.search(query);
	std::vector<Track> result; // FIXME: Needs a vector<pair<SearchID, vector<Track> > > or similar so we can hold network search results
	{
		mutex::scoped_lock lock(client_id_mutex);
		BOOST_FOREACH(LocalTrack lt, local_result) {
			result.push_back( Track(TrackID( client_id, lt.id), lt.metadata ) );
		}
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
	//       then we should do this in a separate thread rather than force the client
	//       to wait.
	while(messageref msg = client_synced_playlist.pop_msg()) {
		try {
			networkhandler.send_server_message(msg);
		}
		catch(Exception e) {}
	}
}

void middle_end::handle_message_request_file(const message_request_file_ref request, bool* done) {
	MetaDataMap md;
	Track q(request->id, md);
	vector<LocalTrack> s = trackdb.search(q);
	bool tl_done(false);
	{
		mutex::scoped_lock lock(file_requests_mutex);
	 	tl_done = *done;
	}
	if(s.size() == 1) {
		if(filesystem::exists(s[0].filename)) {
			filesystem::ifstream f( s[0].filename, std::ios_base::in | std::ios_base::binary );
			std::vector<uint8> data;
			data.resize(1024*40); // 320bpbs
			while(!tl_done) {
				f.read((char*)&data[0], 1024*40);
				uint32 read = f.gcount();
				if(read) {
					std::vector<uint8> vdata;
					std::vector<uint8>::iterator from = data.begin();
					std::vector<uint8>::iterator to = from + read;
					vdata.insert(vdata.begin(), from, to);
					//FIXME: it is impossible to detect if the server is still listening (or even alive)
					message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
					try {
						networkhandler.send_server_message(msg);
					}
					catch(Exception e) {
						tl_done = true;
					}
					//FIXME: this does not take network latency into account (LAN APP!)
					usleep(750000);
					{
						mutex::scoped_lock lock(file_requests_mutex);
						tl_done = *done;
					}
				}
				else { //EOF or Error reading file
					tl_done = true;
				}
			}
			#if 0 // Leave this disabled for now. Do we really want to send 1MB chunks?
			uint32 n = 1024 * 32; // Well over 5sec of 44100KHz 2Channel 16Bit WAV, 32Kb
			std::vector<uint8> data;
			data.resize(1024*1024);
			while(!tl_done) {
				f.read((char*)&data[0], n);
				uint32 read = f.gcount();
				if(read) {
					std::vector<uint8> vdata;
					std::vector<uint8>::iterator from = data.begin();
					std::vector<uint8>::iterator to = from + read;
					vdata.insert(vdata.begin(), from, to);
					//FIXME: it is impossible to detect if the server is still listening (or even alive)
					message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
					try {
						networkhandler.send_server_message(msg);
					}
					catch(Exception e) {
						tl_done = true;
					}

					if(n<1024*1024) {
						n<<=1;
					}
					//FIXME: this does not take network latency into account (LAN APP!)
					usleep(n*10); // Ease up on CPU usage, starts with ~0.3 sec sleep and doubles every loop
					{
						mutex::scoped_lock lock(file_requests_mutex);
						tl_done = *done;
					}
				}
				else { //EOF or Error reading file
					tl_done = true;
				}
			}
			#endif
		}
		else { // Error opening file // FIXME: This is a hack!
			dcerr("Warning: could not open " << s[0].filename << ", Sending as-is...");
			std::vector<uint8> vdata;
			BOOST_FOREACH(char c, s[0].filename.string()) {
				vdata.push_back((uint8)c);
			}
			message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
			try {
				networkhandler.send_server_message(msg);
			} catch(Exception e) {}
		}
	}
	// Send final empty message
	std::vector<uint8> vdata;
	message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));
	try {
		networkhandler.send_server_message(msg);
	} catch(Exception e) {}
	dcerr("Tranfer complete");

	{
		mutex::scoped_lock lock(file_requests_mutex);
		for(vector<file_requests_type>::iterator i = file_requests.begin(); i != file_requests.end(); ++i) {
			if((*i).first.second == this_thread::get_id()) {
				file_requests.erase(i);
				break; // There is only one thread with this ID, and we just invalidated i
			}
		}
		delete done;
	}
}

void middle_end::abort_file_transfer(LocalTrackID id) {
	mutex::scoped_lock fr_lock(file_requests_mutex);
	mutex::scoped_lock t_lock(threads.lock());
	for(vector<file_requests_type>::iterator i = file_requests.begin(); i != file_requests.end(); ++i) {
		if((*i).second == id) {
			*(*i).first.first = true;
			for(int t = 0 ; t < threads.size(); ++t) {
				shared_ptr<thread> pthread = threads[t];
				if(pthread->get_id() == (*i).first.second) {
					fr_lock.unlock();
					t_lock.unlock();
					pthread->interrupt();
					pthread->join(); // Should not take long ...
					break;
				}
			}
			break;
		}
	}
}

void middle_end::playlist_vote_down(Track track) {
	message_vote_ref m = message_vote_ref(new message_vote(track.id, true));
	try {
		networkhandler.send_server_message(m);
	}
	catch(Exception e) {}
}

void middle_end::playlist_vote_up(Track track) {
	message_vote_ref m = message_vote_ref(new message_vote(track.id, false));
	try {
		networkhandler.send_server_message(m);
	}
	catch(Exception e) {}
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
		case message::MSG_ACCEPT: {
			dcerr("Accepted by server");
			const message_accept_ref msg = static_pointer_cast<message_accept>(m);
			{
				mutex::scoped_lock lock(client_id_mutex);
				client_id = msg->cid;
				sig_client_id_changed(client_id);
			}
			mutex::scoped_lock lock(dest_server_mutex);
			if((dest_server == networkhandler.get_target_server_address()) && (dest_server != ipv4_socket_addr())) {
				sig_connect_to_server_success(dest_server, msg->cid);
			}
			else {
				networkhandler.client_disconnect_from_server();
			}
		}; break;
		case message::MSG_DISCONNECT: {
			dcerr("Disconnected from server");
			mutex::scoped_lock s_lock(dest_server_mutex);
			mutex::scoped_lock c_lock(client_id_mutex);
			abort_all_file_transfers();			
			if((dest_server == networkhandler.get_target_server_address()) && (dest_server != ipv4_socket_addr()) && client_id != ClientID(-1)) {
				// We are connected to this particular server
				const message_disconnect_ref msg = static_pointer_cast<message_disconnect>(m);
				client_id = ClientID(-1);
				sig_client_id_changed(client_id);
				sig_disconnected(msg->get_reason());
			}
			else if((client_id == ClientID(-1)) && (networkhandler.get_target_server_address()) == ipv4_socket_addr() && (dest_server != ipv4_socket_addr())) {
				// We're not connected to any server. (and possibly an attempt to connect just failed)
				const message_disconnect_ref msg = static_pointer_cast<message_disconnect>(m);
				sig_connect_to_server_failure(dest_server, msg->get_reason());
			}
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
			if(!sig_update_playlist.empty()) {
				IPlaylistRef playlist = sig_update_playlist();
				const message_playlist_update_ref msg = static_pointer_cast<message_playlist_update>(m);
				msg->apply(playlist);
			}
		}; break;
//		case message::MSG_QUERY_TRACKDB: {
///*			message_query_trackdb_ref qr = static_pointer_cast<message_query_trackdb>(m);
//			vector<Track> res;
//			BOOST_FOREACH(LocalTrack t, trackDB.search(qr->search)) {
//				Track tr(TrackID(gmpmpc_client_id, t.id), t.metadata);
//				res.push_back(tr);
//			}
//			message_query_trackdb_result_ref result(new message_query_trackdb_result(qr->qid, res));
//			gmpmpc_network_handler->send_server_message(result);*/
//		}; break;
//		//case message::MSG_QUERY_TRACKDB_RESULT: {}; break;
		case message::MSG_REQUEST_FILE: {
			message_request_file_ref msg = static_pointer_cast<message_request_file>(m);
			mutex::scoped_lock lock(file_requests_mutex);
			bool *b = new bool(false);
			thread::id tid = threads.create_thread(bind(&middle_end::handle_message_request_file, this, msg, b))->get_id();
			file_requests.push_back(pair<pair<bool*, thread::id>, LocalTrackID>(pair<bool* , thread::id>(b, tid), msg->id.second));
		}; break;
		case message::MSG_REQUEST_FILE_RESULT: {
			message_request_file_result_ref msg = static_pointer_cast<message_request_file_result>(m);
			mutex::scoped_lock lock(client_id_mutex);
			if(msg->id.first == client_id) {
				abort_file_transfer(msg->id.second);
			}
		}; break;
		default: {
			dcerr("Not handling message of type " << m->get_type());
		} break;
	}
}
