#include "middle_end.h"
#include "error-handling.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp> // FIXME: should this not be included by <boost/filesystem.hpp> ?
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using namespace boost;

#ifdef WIN32
	#include <shlwapi.h>
	#include <shlobj.h>
	#include <stdio.h>
	#pragma comment(lib,"shlwapi.lib")
#endif
middle_end::middle_end()
: networkhandler(TCP_PORT_NUMBER),
  client_id(-1)
{
	networkhandler.server_list_update_signal.connect(boost::bind(&middle_end::handle_sig_server_list_update, this, _1));
	networkhandler.client_message_receive_signal.connect(boost::bind(&middle_end::handle_received_message, this, _1));

#ifdef _WIN32
	uint32 locations[] = {
		CSIDL_COMMON_MUSIC,
		CSIDL_MYMUSIC,
	};

	BOOST_FOREACH(uint32 location, locations) {
		TCHAR szPath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath(NULL, location, NULL, 0, szPath ))) {
			trackdb.add_directory(szPath);
		}
	}

	char szBuffer[1024];
	::GetLogicalDriveStrings(1024, szBuffer);
	char *pch = szBuffer;
	while(*pch) {
		string drive(pch);

		/* English locations */
		trackdb.add_directory(drive + "Music\\");
		trackdb.add_directory(drive + "Stuff\\music\\");
		trackdb.add_directory(drive + "Mp3\\");
		trackdb.add_directory(drive + "My Documents\\My Music\\");

		/* Foreign languages */
		trackdb.add_directory(drive + "Musica\\");
		trackdb.add_directory(drive + "Multimedia\\Musica\\");

		pch = &pch[strlen(pch) + 1];
	}

	trackdb.add_directory("f:\\home\\dafox\\sharedfolder\\music\\");
#endif // WIN32
	trackdb.add_directory("/home/dafox/sharedfolder/music/");

	networkhandler.start();
	sig_search_tracks.connect(boost::bind(&middle_end::handle_msg_query_trackdb_query_result, this, _1, _2));
	client_synced_playlist.sig_send_message.connect(boost::bind(&network_handler::send_server_message, &networkhandler, _1));

#ifdef DEBUG
	next_search_id = 0; // Intentionally not initialized.
#endif
}

void middle_end::trackdb_add(boost::filesystem::path path) {
	trackdb.add_directory(path);
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

void middle_end::_search_tracks(SearchID search_id, const Track query, boost::shared_ptr<boost::mutex> pm) {
	mutex::scoped_lock lock(search_mutex); //FIXME: Searching should be possible concurrently
	std::vector<LocalTrack> local_result = trackdb.search(query);
	std::vector<Track> result; // FIXME: Needs a vector<pair<SearchID, vector<Track> > > or similar so we can hold network search results
	{
		mutex::scoped_lock lock(client_id_mutex);
		BOOST_FOREACH(LocalTrack lt, local_result) {
			result.push_back( Track(TrackID( client_id, lt.id), lt.metadata ) );
		}
	}

	{
		mutex::scoped_lock lock(*pm);
		sig_search_tracks(search_id, result);
	}
}

boost::shared_ptr<mutex::scoped_lock> middle_end::search_tracks(const Track query, SearchID* id) {
	return search_tracks(query, false, id);	
}

boost::shared_ptr<mutex::scoped_lock> middle_end::search_tracks(const Track query, bool local_search_only, SearchID* id) {
	{
	mutex::scoped_lock lock(next_search_id_mutex);
	*id = next_search_id++;
	}

	boost::shared_ptr<boost::mutex> pm(new boost::mutex);
	boost::shared_ptr<boost::mutex::scoped_lock> plock(new mutex::scoped_lock(*pm));
	threads.create_thread(boost::bind(&middle_end::_search_tracks, this, *id, query, pm));

	if(!local_search_only) {
		message_query_trackdb_ref msg(new message_query_trackdb(*id, query));
		try {
			networkhandler.send_server_message(msg);
		}
		catch(Exception e) {}
	}
	//try { // create thread
	//} catch(thread_resource_error e) {
	//	// FIXME: Figure out if this is an error which we should be catching or not
	//	//        thread_group does not remove a thread object when the thread
	//	//        finishes execution, so this may just indicate an out-of-memory-error,
	//	//        in which case there may be no sense in waiting for memory to become
	//	//        available...
	//	return SearchID(-1);
	//}
	return plock;
}

void middle_end::handle_msg_query_trackdb_query_result(const SearchID id, const std::vector<Track>& tracklist) {
	mutex::scoped_lock lock(trackdb_queries_mutex);
	std::map<SearchID, uint32>::iterator i = trackdb_queries.find(id);
	if(i != trackdb_queries.end()) {
		message_query_trackdb_result_ref msg(new message_query_trackdb_result(i->second, tracklist));
		networkhandler.send_server_message(msg);
		trackdb_queries.erase(i);
	}
}

void middle_end::mylist_append(Track track) {
	client_synced_playlist.append(track);

	filesystem::ofstream f;

	//FIXME: If this is going to take a while (e.g. slow connection / lots of tracks)
	//       then we should do this in a separate thread rather than force the client
	//       to wait.
	client_synced_playlist.sync();
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

			const int BYTES_TO_SEND(1024*4);
			std::vector<uint8> data(BYTES_TO_SEND);
			uint32 bytes_sent = 0;
			while(!tl_done) {
				f.read((char*)&data[0], BYTES_TO_SEND);
				uint32 read = f.gcount();
				posix_time::ptime time_a(posix_time::microsec_clock::universal_time());
				if(read) {
					std::vector<uint8> vdata;
					std::vector<uint8>::iterator from = data.begin();
					std::vector<uint8>::iterator to = from + read;
					vdata.insert(vdata.begin(), from, to);
					message_request_file_result_ref msg(new message_request_file_result(vdata, request->id));

					try {
						networkhandler.send_server_message(msg);
					}
					catch(Exception e) {
						tl_done = true;
					}
					posix_time::ptime time_b(posix_time::microsec_clock::universal_time());

					// Common WAV formats:
					// 8KHz 8bit 1-channel        ~= 7.8125 kb/s
					// 22.05KHz 16bit 2-channels  ~= 86.1328125 kb/s
					// 44.1KHz 16bit 2-channels   ~= 172.265625 kb/s
					// 48khz 16bit 2-channels     ~= 187.5 kb/s
					// Uncommon WAV formats:
					// 48kHz 24bit 2-channels     ~= 281.25 kb/s
					// 48kHz 32bit 2-channels     ~= 375 kb/s
					// 96kHz 32bit 2-channels     ~= 750 kb/s
					//
					// So choose 256kb/s as a target transfer speed.
					// This is 256*8/320 = 6.4 times higher than the highest
					// officially supported MP3 bitrate, and should even accomodate
					// most flac encoded files.					
					bytes_sent += read;
					if(bytes_sent > 1024 * 256) { // After 256k we start slowing down
						posix_time::time_duration time_elapsed = time_b - time_a;
						// we target 256KB/ps
						uint64 bps = (256 * 1024) / BYTES_TO_SEND; // number of buffers per second
						uint64 target_duration = 1000000 / bps;
						target_duration = time_elapsed.total_microseconds(); // time left for this buffer
						posix_time::time_duration sleep_time = posix_time::microseconds(target_duration);
						if(sleep_time > posix_time::microseconds(0)) {
							try {
								this_thread::sleep(sleep_time);
							}
							catch(boost::thread_interrupted /*e*/) {} // We will only sleep less because of this, no need to sleep 'the rest'
						}
					}
					time_a = posix_time::microsec_clock::universal_time();

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
				boost::shared_ptr<boost::thread> pthread = threads[t];
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
			mutex::scoped_lock s_lock(dest_server_mutex);
			mutex::scoped_lock c_lock(client_id_mutex);
			abort_all_file_transfers();
			if(dest_server != ipv4_socket_addr()) {
				// We have at least tried to connect to a server.
				const message_disconnect_ref msg = static_pointer_cast<message_disconnect>(m);
				if(client_id != ClientID(-1) && (dest_server == networkhandler.get_target_server_address())) { // We are connected to this particular server
					client_id = ClientID(-1);
					sig_client_id_changed(client_id);
					sig_disconnected(msg->get_reason());
				}
				else { // We're not connected to any server. (and possibly an attempt to connect just failed)
					sig_connect_to_server_failure(dest_server, msg->get_reason());
				}
			}
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
			if(!sig_update_playlist.empty()) {
				IPlaylistRef playlist = sig_update_playlist();
				const message_playlist_update_ref msg = static_pointer_cast<message_playlist_update>(m);
				msg->apply(playlist);
			}
		}; break;
		case message::MSG_QUERY_TRACKDB: {
			const message_query_trackdb_ref msg = static_pointer_cast<message_query_trackdb>(m);
			SearchID id;
			mutex::scoped_lock lock(trackdb_queries_mutex);
			boost::shared_ptr<boost::mutex::scoped_lock> plock = search_tracks(msg->search, true, &id);
			trackdb_queries[id] = msg->qid;
		}; break;
		case message::MSG_QUERY_TRACKDB_RESULT: {
			const message_query_trackdb_result_ref msg = static_pointer_cast<message_query_trackdb_result>(m);
			sig_search_tracks(SearchID(msg->qid), msg->result);
		}; break;
		case message::MSG_REQUEST_FILE: {
			message_request_file_ref msg = static_pointer_cast<message_request_file>(m);
			mutex::scoped_lock lock(file_requests_mutex);
			bool *b = new bool(false);
			boost::thread::id tid = threads.create_thread(boost::bind(&middle_end::handle_message_request_file, this, msg, b))->get_id();
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
