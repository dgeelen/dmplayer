#include "cross-platform.h"
#include "mpmpd.h"
#include "network-handler.h"
#include "error-handling.h"
#include <boost/program_options.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include "audio/audio_controller.h"
#include "playlist_management.h"
#include "boost/filesystem.hpp"
#include "playlist_management.h"
#include <boost/thread/mutex.hpp>
#include "synced_playlist.h"
#include "util/StrFormat.h"
#include "util/UPXFix.h"
#include <algorithm>

namespace po = boost::program_options;
using namespace std;

class Client {
	public:
		Client() : id(ClientID(-1)), zero_sum(0.0) {}
		Client(ClientID id_) {
			id = id_;
			zero_sum = 0.0;
		}

		ClientID id;
		double zero_sum;
		PlaylistVector wish_list;
};
typedef boost::shared_ptr<Client> Client_ref;

class ServerDataSource : public IDataSource {
	public:
		ServerDataSource() {
			position = 0;
			data_exhausted = false;
			wait_for_data = true;
			finished = false;
		}

		virtual uint32 getpos() {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			return position;
		}

		virtual bool exhausted() {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			return (!wait_for_data) && data_exhausted || finished;
		}

		virtual void reset() {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			position = 0;
			data_exhausted = data_buffer.size()==0;
		}

		virtual uint32 getData(uint8* buffer, uint32 len) {
			size_t n = 0;
			size_t avail;
			{
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			avail = size_t(data_buffer.size() - position);
			if (!finished)
				n = std::min(size_t(len), avail);
			if (n>0)
				memcpy(buffer, &data_buffer[position], n);
			}
			position += n;
			avail    -= n;
			if(avail==0) {
				data_exhausted = true;
				if(wait_for_data) {
					//FIXME: Seems like a long time to sleep, but needed
					//       otherwise the portaudio callback with PRIO_REALTIME
					//       still takes over the system :-(
					//std::cout << "Waiting for data... ZZZzzz" << std::endl;
					boost::this_thread::sleep(boost::posix_time::seconds(1));
					//std::cout << "Huh what?" << std::endl;
					boost::this_thread::yield();
					//std::cout << "meh" << std::endl;
				}
			}
			return n;
		}

		void appendData(std::vector<uint8>& data) {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			data_buffer.insert(data_buffer.end(), data.begin(), data.end());
			data_exhausted = false;
		}

		void stop() {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			finished = true;
			position = data_buffer.size();
		}

		void set_wait_for_data(bool b) {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			wait_for_data = b;
		}

	private:
		boost::mutex data_buffer_mutex;
		std::vector<uint8> data_buffer;
		bool data_exhausted;
		bool wait_for_data;
		bool finished;
		long position;
};

class Server {
	public:
		Server(int listen_port, string server_name)
			: networkhandler(listen_port, server_name)
			, server_message_receive_signal_connection(networkhandler.server_message_receive_signal.connect(boost::bind(&Server::handle_received_message, this, _1, _2)))
			, data_source_renewed_barrier(2)
		{
			playlist.sig_send_message.connect(boost::bind(&network_handler::send_message_allclients, &networkhandler, _1));
			// The following constant was obtained by scanning my personal music library,
			// counting only songs with a duration greater than 00:01:30 and less than
			// 00:10:00. This yielded a total duration of 3 days, 14 hours, 14 minutes and
			// 2 seconds, or 310442 seconds, for a total of 1461 songs. Thus on average
			// a song lasted 212.486~ seconds, or just over 3.5 minutes. For esthetic reasons
			// I thus chose 210 (exactly 3.5 minutes) as a safe default.
			average_song_duration = 210;
			ac.playback_finished.connect(boost::bind(&Server::next_song, this, _1));
			#ifdef DEBUG
				next_query_id = 0; // Intentionally not initialized.
			#endif
			dcerr("Starting network_handler");
			networkhandler.start();
			dcerr("Server initialized");
		}

		~Server() {
			dcerr("shutting down");
			dcerr("stopping networkhandler");
			networkhandler.send_message_allclients(messageref(new message_disconnect("Server is shutting down.")));
			networkhandler.stop();

			server_message_receive_signal_connection.disconnect();

			dcerr("Stopping playback");
			ac.playback_finished.disconnect(boost::bind(&Server::next_song, this, _1));
			ac.stop_playback();
			dcerr("Joining add_datasource_thread");
			{
				boost::mutex::scoped_lock add_datasource_thread_lock(add_datasource_thread_mutex);
				boost::mutex::scoped_lock server_datasource_lock(server_datasource_mutex);
				if(server_datasource) {
					server_datasource->set_wait_for_data(false);
					server_datasource->stop();
				}
				add_datasource_thread.join();
			}
			dcerr("shut down");
		}

		void next_song(uint32 playtime_secs) {
			cout << "Next song, playtime was " << playtime_secs << " seconds" << endl;
			if((vote_min_penalty && playtime_secs < average_song_duration * 0.2) || (playtime_secs < 15)) {
				cout << "issueing a penalty of " << uint32(average_song_duration) << " seconds" << endl;
				playtime_secs += uint32(average_song_duration);
			}

			if(!vote_min_penalty) { // Update rolling average of last 16 songs
				average_song_duration = (average_song_duration * 15.0 + double(playtime_secs)) / 16.0;
				cout << "average song duration now at " << uint32(average_song_duration) << " seconds" << endl;
			}
			vote_min_penalty = false;

			{
				boost::mutex::scoped_lock current_track_lock(current_track_mutex);
				boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);
				vote_min_list.erase(current_track.id);
			}

			Track t;
			{
				boost::mutex::scoped_lock current_track_lock(current_track_mutex);
				t = current_track;
			}
			{
			boost::mutex::scoped_lock clients_lock(clients_mutex);
			update_zero_sum(t, playtime_secs, clients_lock);
			}
			recalculateplaylist();
			cue_next_track();
		}

		void update_zero_sum(const Track& target_track, const uint32 playtime_secs, boost::mutex::scoped_lock& clients_lock) {
			vector<ClientID> has_song;
			vector<ClientID> does_not_have_song;
// 			Track t = current_track;
			BOOST_FOREACH(Client_ref c, clients) {
				uint32 pos = 0;
				for (;pos < c->wish_list.size(); ++pos)
					if (c->wish_list.get(pos).id == target_track.id)
						break;
				if (pos != c->wish_list.size()) {
					c->wish_list.remove(pos);
					has_song.push_back(c->id);
					// TODO: do something with weights!
				}
				else {
					does_not_have_song.push_back(c->id);
				}
			}

			// TODO: do something with weights and playtime_secs
			double avg_min  = double(playtime_secs)/has_song.size();
			double avg_plus = double(playtime_secs)/clients.size();
			if((has_song.size() == 0) || (clients.size() == 0)) { // Should only happen if we're (just have) removing a client
				cout << "WARNING: Division by zero, ZeroSum compromised!!";
				cout << "has_song.size()==" << has_song.size() << " clients.size()==" << clients.size() << " avg_min==" << avg_min << " avg_plus==" << avg_plus;
				avg_plus = avg_min = 0.0; //FIXME : ugly hax
			}
			BOOST_FOREACH(ClientID& id, has_song) {
				double old = (*clients.find(id))->zero_sum;
				(*clients.find(id))->zero_sum -= avg_min;
				(*clients.find(id))->zero_sum += avg_plus;
				cout << "(min)  for " << STRFORMAT("%08x", id) << ": " << old << " -> " << (*clients.find(id))->zero_sum << endl;
			}
			BOOST_FOREACH( ClientID& id, does_not_have_song) {
				double old = (*clients.find(id))->zero_sum;
				(*clients.find(id))->zero_sum += avg_plus;
				cout << "(plus) for " << STRFORMAT("%08x", id) << ": " << old << " -> " << (*clients.find(id))->zero_sum << endl;
			}
		}

		void playlist_sync_loop() {
			server_thread_start_barrier.wait();
			while(playlist_sync_loop_running) {
				messageref m;
				do {
					boost::mutex::scoped_lock lock(playlist_mutex);
					m = playlist.pop_msg();
					if(m)
						networkhandler.send_message_allclients(m);
				} while(m);
				usleep(100*1000);
			}
		}

		boost::barrier data_source_renewed_barrier;
		void cue_next_track() {
			boost::mutex::scoped_lock lock(add_datasource_thread_mutex);
			if(add_datasource_thread.get_id() != boost::this_thread::get_id())
				add_datasource_thread.join(); //FIXME: probably ac.set_data_source() should throw an exception
			boost::thread t(boost::bind(&Server::add_datasource, this));
			add_datasource_thread.swap(t);
			data_source_renewed_barrier.wait();
		}

		void add_datasource() {
			// Starting playback of a new song is done in a separate thread since
			// set_data_source() may take a while.
			// FIXME: "a while" == potentially forever, in which case no new song
			//        can be started. ever.
			boost::shared_ptr<ServerDataSource> ds;
			{
				std::map<Track, uint32> penalty_list;
				boost::mutex::scoped_lock clients_lock(clients_mutex);
				boost::mutex::scoped_lock playlist_lock(playlist_mutex);
				boost::mutex::scoped_lock server_datasource_lock(server_datasource_mutex);
				boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);
				bool found_track = false;
				uint32 t_index = 0;
				dcerr("While");
				while(!found_track && (t_index < playlist.size())) {
					std::map<TrackID, std::set<ClientID> >::iterator i = vote_min_list.find(playlist.get(t_index).id);
					// if more than half of all clients voted for this song
					if((i != vote_min_list.end()) && ((i->second.size() * 2) > clients.size())) {
						// mark track playlist[t_index] for removal from all wish_lists
						// FIXME: average song duration does not have a mutex yet (does it need one?)
						penalty_list[playlist.get(t_index)] += average_song_duration;
					}
					else {
						found_track = true;
						break;
					}
					++t_index;
				}

				dcerr("foreach");
				typedef std::pair<Track, uint32> vtype;
				BOOST_FOREACH(vtype p, penalty_list) {
					update_zero_sum(p.first, p.second, clients_lock);
					// remove track playlist[t_index] from all wish_lists
					// FIXME: Assumes recalculateplaylist() merges multiple entries into a single entry.
					BOOST_FOREACH(Client_ref c, clients) {
						for(uint32 i=0; i < c->wish_list.size(); ++i) {
							if(c->wish_list.get(i).id == p.first.id) {
								c->wish_list.remove(i);
								break;
							}
						}
					}
					vote_min_list.erase(vote_min_list.find(p.first.id)); // no longer in any wish_list, remove also from vote_min_list
				}

				dcerr("final");
				if(t_index < playlist.size()) {
					server_datasource = boost::shared_ptr<ServerDataSource>(new ServerDataSource());
					current_track = playlist.get(t_index);
					message_request_file_ref msg(new message_request_file(current_track.id));
					dcerr("requesting " << STRFORMAT("%08x:%08x", current_track.id.first, current_track.id.second));
					networkhandler.send_message(current_track.id.first, msg);
				}
				else {
					current_track = Track();
					server_datasource.reset();
				}
				ds = server_datasource;
			}
			recalculateplaylist();
			data_source_renewed_barrier.wait();
			ac.set_data_source(ds);
			ac.start_playback(); // FIXME: do this here?
		}

		void remove_client(ClientID id) {
			boost::mutex::scoped_lock clock(clients_mutex);
			cout << "Removing client " << STRFORMAT("%08x", id) << std::endl;
			if (clients.find(id) == clients.end())
				return;
			{
				boost::mutex::scoped_lock lock(server_datasource_mutex);
				if(current_track.id.first == id && server_datasource) { // Client which is serving current file disconnected!
					server_datasource->set_wait_for_data(false);
				}
			}

			{
				boost::mutex::scoped_lock lock(vote_min_list_mutex);
				std::vector<TrackID> empty_vote_tracks;
				typedef std::pair<TrackID, std::set<ClientID> > vtype;
				BOOST_FOREACH(vtype i, vote_min_list) {           // For each (trackid, {client})
					if(i.second.find(id) != i.second.end()) {     // If the disconnecting client voted for trackid
						i.second.erase(i.second.find(id));        // Remove him/her from the list of voters
						if(i.second.size() == 0) {                // and if he/she was the last voter
							empty_vote_tracks.push_back(i.first);
						}
					}
				}
				BOOST_FOREACH(TrackID id, empty_vote_tracks) {
					vote_min_list.erase(vote_min_list.find(id));  // remove trackid from vote_min_list
				}
			}

			{
				boost::mutex::scoped_lock lock(outstanding_query_result_list_mutex);
				std::map<uint32, std::pair<std::pair<ClientID, uint32>,  std::set<ClientID> > >::iterator i;
				std::set<uint32> completed_queries;
				for(i = outstanding_query_result_list.begin(); i != outstanding_query_result_list.end(); ++i) {
					std::pair<ClientID, uint32>& query_id = i->second.first;
					std::set<ClientID>& queried_clients = i->second.second;
					if(id == query_id.first) {
						completed_queries.insert(i->first);
					}
					std::set<ClientID>::iterator c = queried_clients.find(id);
					if(c != queried_clients.end()) {
						queried_clients.erase(c);
						if(queried_clients.size() == 0) {
							completed_queries.insert(i->first);
						}
					}
				}
				for(std::set<uint32>::iterator i = completed_queries.begin(); i != completed_queries.end(); ++i) {
					outstanding_query_result_list.erase(outstanding_query_result_list.find(*i));
				}
			}

			Client_ref cr = *clients.find(id);
			// check if somebody else is still listening.
			bool has_listeners = false;
			BOOST_FOREACH(Client_ref c, clients) {
				if(cr->id != c->id) {
					for(uint32 i = 0 ; i < c->wish_list.size(); ++i) {
						if(c->wish_list.get(i).id == current_track.id) {
							has_listeners = true;
							break;
						}
					}
				}
				for(unsigned int i=0; i < c->wish_list.size(); ++i) {
					if(c->wish_list.get(i).id.first == id) { // No way to retrieve this song from this client now...
						c->wish_list.remove(i);
						--i;
					}
				}
			}

			// Do some magic with 'ghost' clients to prevent zero_sum from becoming corrupt
			// due to this client now leaving. (Might cause 'has_song' to become an empty set)
			ClientID invalid_cid(-1);
			ClientMap::iterator ghost = clients.insert(Client_ref(new Client(invalid_cid))).first;
			(*ghost)->zero_sum = cr->zero_sum;
			(*ghost)->wish_list.append(current_track);
			clients.erase(id);
			disconnected_clients[id] = cr;
			if(!has_listeners) { // Cue next song since no-one wants to hear this now
				// NOTE:
				//  * Also calls next_song which is important since if has_song becomes empty
				//    zero_sum is compromised.
				//  * Will not try to enque a song from this client since we just removed all
				//    his songs from all wish_lists.
				//  * Will not race msg_playlist_update because of the ghost client we just added.
				clock.unlock(); // make sure we don't hold any locks when we start calling other functions
				ac.stop_playback();
				clock.lock();
				ghost = clients.find(invalid_cid); // iterator may have become invalid, refresh it
			}
			cr->zero_sum = (*ghost)->zero_sum;
			clients.erase(ghost);

			double total = cr->zero_sum;
			cout << "This clients zero_sum was:" << total << std::endl;
			BOOST_FOREACH(Client_ref i, clients) {
				double old = i->zero_sum;
				i->zero_sum += total/clients.size();
				cout << "(dc)  for " << STRFORMAT("%08x", i->id) << ": " << old << " -> " << i->zero_sum << std::endl;
			}
			clock.unlock();
			recalculateplaylist();
		}

		void handle_received_message(const messageref m, ClientID id) {
			bool recalculateplaylist = false;
			bool is_known_client = false;
			{
				boost::mutex::scoped_lock lock_clients(clients_mutex);
				is_known_client = (clients.find(id) != clients.end());  // Ensure that we know this client,
			}
			if((m->get_type() ==  message::MSG_CONNECT) || is_known_client) { // unless it is a genuine request.
				switch(m->get_type()) {
					case message::MSG_CONNECT: {
						dcerr("Received a MSG_CONNECT from " << STRFORMAT("%08x", id));
						{
							boost::mutex::scoped_lock lock_clients(clients_mutex);
							std::map<ClientID, Client_ref>::iterator i = disconnected_clients.find(id);
							bool is_reconnect = true;
							if(i == disconnected_clients.end()) {
								disconnected_clients[id] = Client_ref(new Client(id));
								i = disconnected_clients.find(id);
								is_reconnect = false;
							}
							Client_ref cr = i->second;
							disconnected_clients.erase(i);
							double total = cr->zero_sum;
							string str("Client ");
							if(is_reconnect)
								str += "re-";
							str += STRFORMAT("connect detected for %x, zero_sum: %i\n", id, total);
							cout << str;
							BOOST_FOREACH(Client_ref i, clients) {
								double old = i->zero_sum;
								i->zero_sum -= total/clients.size();
								cout << "(dc)  for " << STRFORMAT("%08x", i->id) << ": " << old << " -> " << i->zero_sum << std::endl;
							}
							clients.insert(cr);
						}
						boost::mutex::scoped_lock lock_playlist(playlist_mutex);
						networkhandler.send_message(id, messageref(new message_playlist_update(playlist)));
					} break;
					case message::MSG_ACCEPT: {
						dcerr("Received a MSG_ACCEPT from " << STRFORMAT("%08x", id));
					}; break;
					case message::MSG_DISCONNECT: {
						dcerr("Received a MSG_DISCONNECT from " << STRFORMAT("%08x", id));
						remove_client(id);
					} break;
					case message::MSG_PLAYLIST_UPDATE: {
						//dcerr("Received a MSG_PLAYLIST_UPDATE from " << STRFORMAT("%08x", id));
						message_playlist_update_ref msg = boost::static_pointer_cast<message_playlist_update>(m);
						boost::mutex::scoped_lock lock_clients(clients_mutex);
						PlaylistVector& pl = ((*clients.find(id))->wish_list);
						if((pl.size() < 100) ||
							(    (msg->get_type()!=message_playlist_update::UPDATE_APPEND_ONE)
							  && (msg->get_type()!=message_playlist_update::UPDATE_APPEND_MANY)
							  && (msg->get_type()!=message_playlist_update::UPDATE_INSERT_ONE)
							  && (msg->get_type()!=message_playlist_update::UPDATE_INSERT_MANY))
							)
						{
							msg->apply(pl);
							recalculateplaylist = true;
						}
						else {
							dcerr("Ignoring playlist update, playlist full."); // TODO: Penalty?
						}
					}; break;
					case message::MSG_QUERY_TRACKDB: {
						//dcerr("Received a MSG_QUERY_TRACKDB from " << STRFORMAT("%08x", id));
						message_query_trackdb_ref msg = boost::static_pointer_cast<message_query_trackdb>(m);
						boost::mutex::scoped_lock lock(outstanding_query_result_list_mutex);
						uint32 qid = next_query_id++;
						std::pair<ClientID, uint32> query_id(id, msg->qid);
						msg->qid = qid;
						std::set<ClientID> queried_clients;
						BOOST_FOREACH(Client_ref cref, clients) {
							if(cref->id != id) {
								networkhandler.send_message(cref->id, msg);
								queried_clients.insert(cref->id);
							}
						}
						std::pair<std::pair<ClientID, uint32>, std::set<ClientID> > outstanding_queries(query_id, queried_clients);
						outstanding_query_result_list[qid] = outstanding_queries;
					}; break;
					case message::MSG_QUERY_TRACKDB_RESULT: {
						//dcerr("Received a MSG_QUERY_TRACKDB_RESULT from " << STRFORMAT("%08x", id));
						message_query_trackdb_result_ref msg = boost::static_pointer_cast<message_query_trackdb_result>(m);
						boost::mutex::scoped_lock lock(outstanding_query_result_list_mutex);
						std::map<uint32, std::pair<std::pair<ClientID, uint32>,  std::set<ClientID> > >::iterator i;
						i = outstanding_query_result_list.find(msg->qid);
						if(i != outstanding_query_result_list.end()) {
							std::pair<ClientID, uint32>& query_id = i->second.first;
							std::set<ClientID>& queried_clients = i->second.second;
							std::set<ClientID>::iterator c = queried_clients.find(id);
							if(c != queried_clients.end()) {
								queried_clients.erase(c);
								msg->qid = query_id.second;
								networkhandler.send_message(query_id.first, msg);
								if(queried_clients.size() == 0) {
									outstanding_query_result_list.erase(i);
								}
							}
						}
					}; break;
					case message::MSG_REQUEST_FILE: {
						dcerr("Received a MSG_REQUEST_FILE from " << STRFORMAT("%08x", id));
					}; break;
					case message::MSG_REQUEST_FILE_RESULT: {
						//dcerr("Received a MSG_REQUEST_FILE_RESULT from " << STRFORMAT("%08x", id));
						message_request_file_result_ref msg = boost::static_pointer_cast<message_request_file_result>(m);
						if(current_track.id != msg->id) break;
						if(msg->data.size()) {
							boost::mutex::scoped_lock lock(server_datasource_mutex);
							if(server_datasource)
								server_datasource->appendData(msg->data);
						}
						else { // This was the last message, we have received the full file.
							boost::mutex::scoped_lock lock(server_datasource_mutex);
							if(server_datasource)
								server_datasource->set_wait_for_data(false);
						}
					}; break;
					case message::MSG_VOTE: {
						dcerr("Received a MSG_VOTE from " << STRFORMAT("%08x", id));
						message_vote_ref msg = boost::static_pointer_cast<message_vote>(m);
						if(msg->is_min_vote) {
							boost::mutex::scoped_lock clients_lock(clients_mutex);
							boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);
							vote_min_list[msg->id].insert(id);
							if((current_track.id==msg->id) && (vote_min_list[msg->id].size()*2) > clients.size()) {
								boost::mutex::scoped_lock server_datasource_lock(server_datasource_mutex);
								if(server_datasource) {
									/* Notify sender to stop sending */
									vector<uint8> empty;
									message_request_file_result_ref msg(new message_request_file_result(empty, current_track.id));
									networkhandler.send_message(current_track.id.first, msg);
									vote_min_list.erase(current_track.id); // Don't spam sender
									vote_min_penalty = true; // Current song will incur a penalty
									clients_lock.unlock();
									vote_min_list_lock.unlock();
									server_datasource_lock.unlock();
									ac.stop_playback();      // calls next_song
								}
							}


							//		current_track = Track();
							//		server_datasource.reset();
							//		ac.set_data_source(server_datasource);


							//boost::mutex::scoped_lock lock(vote_min_list_mutex);
							//vote_min_list[msg->id].insert(id);
							//boost::mutex::scoped_lock clients_lock(clients_mutex);
							//if((current_track.id==msg->id) && (vote_min_list[msg->id].size()*2) > clients.size())
							//	recalculateplaylist=true;














	// 						* TODO *
	// 						bool is_in_playlist = false;
	// 						BOOST_FOREACH( , playlist) {
	// 						}
	// 						if(is_in_playlist) {
	// 							vote_min_list[msg->getID()].insert(id);
	// 						}
						}
					}; break;
					default: {
						dcerr("Ignoring unknown message of type " << m->get_type() << " from " << STRFORMAT("%08x", id));
					} break;
				}
			}
			else {
				networkhandler.send_message(id, messageref(new message_disconnect("You are not connected! Go away.")));
			}
			if(recalculateplaylist) {
				this->recalculateplaylist();
				boost::shared_ptr<ServerDataSource> ds;
				{
					boost::mutex::scoped_lock lock(server_datasource_mutex);
					ds = server_datasource;
				}
				if(!ds) {
					cue_next_track();
				}
			}
		}
	private:
		boost::mutex playlist_mutex;
		boost::mutex current_track_mutex;
		Track current_track;
		network_handler networkhandler;
		SyncedPlaylist playlist;
		AudioController ac;
		typedef boost::multi_index_container<
			Client_ref,
			boost::multi_index::indexed_by<
				// non_unique because we need to insert multiple clients with an invalid id at times
				boost::multi_index::ordered_non_unique<boost::multi_index::member<Client, ClientID, &Client::id> >
			>
		> ClientMap;
		ClientMap clients;
		std::map<ClientID, Client_ref> disconnected_clients;
		boost::mutex vote_min_list_mutex;
		std::map<TrackID, std::set<ClientID> > vote_min_list;
		bool vote_min_penalty;
		boost::mutex clients_mutex;

		boost::mutex                        add_datasource_thread_mutex;
		boost::thread                       add_datasource_thread;
		boost::signals::connection          server_message_receive_signal_connection;
		boost::mutex                        server_datasource_mutex;
		boost::shared_ptr<ServerDataSource> server_datasource;

		boost::mutex outstanding_query_result_list_mutex;
		uint32 next_query_id;
		std::map<uint32, std::pair<std::pair<ClientID, uint32>,  std::set<ClientID> > > outstanding_query_result_list;

		/* Some playback related variables*/
		//bool add_datasource;
		double average_song_duration;

		struct sort_by_zero_sum_decreasing {
			bool operator()(const std::pair<Client_ref, uint32>& l, const std::pair<Client_ref, uint32>& r) const {
				return l.first->zero_sum > r.first->zero_sum;
			}
		};

		void recalculateplaylist() {
			std::vector<Track> new_playlist;
			{
			boost::mutex::scoped_lock clients_lock(clients_mutex);
			boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);

			std::vector<std::pair<Client_ref, uint32> > client_list;
			uint32 maxsize = 0;
			BOOST_FOREACH(Client_ref c, clients) {
				if (c->wish_list.size() > 0)
					client_list.push_back(std::pair<Client_ref, uint32>(c, 0));
				if(c->wish_list.size() > maxsize)
					maxsize = c->wish_list.size();
			}

			std::sort(client_list.begin(), client_list.end(), sort_by_zero_sum_decreasing() );
			bool done = false;
			while (!done) {
				done = true;
				typedef std::pair<Client_ref, uint32> vt;
				BOOST_FOREACH(vt& i, client_list) {
					if (i.second < i.first->wish_list.size()) {
						done = false;
						const Track& t(i.first->wish_list.get(i.second++));
						new_playlist.push_back(t);
					}
				}
			}
			}

			{
			boost::mutex::scoped_lock playlist_lock(playlist_mutex);
			playlist.clear();
			playlist.append(new_playlist);
			}

			playlist.sync();
		}
};

int main_impl(int argc, char* argv[])
{
	int listen_port;
	string filename;
	string server_name;
	string musix;
	string findtext;
	bool showhelp;
	cout << "starting mpmpd V" MPMP_VERSION_STRING
	#ifdef DEBUG
	     << "   [Debug Build]"
	#endif
	     << endl;

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", po::bool_switch(&showhelp)                   , "produce help message")
			("port", po::value(&listen_port)->default_value(TCP_PORT_NUMBER), "listen port for daemon (TCP part)")
			("file", po::value(&filename)->default_value("")      , "file to play (Debug for fmod lib)")
			("name", po::value(&server_name)->default_value("mpmpd V" MPMP_VERSION_STRING), "Server name")
			("musix", po::value(&musix)->default_value("")      , "directory to add music from")
			("find", po::value(&findtext)->default_value("")      , "text to find in database")
	;

	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (showhelp) {
		cout << desc << endl;
		return 1;
	}

	if(filename != "") {
		AudioController ac;
		ac.test_functie(filename);
		ac.start_playback();
		cout << "Press any key to quit" << endl;
		getchar();
		return 0;
	}

	Server svr(listen_port, server_name);

	cout << "Press any key to quit" << endl;
	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
