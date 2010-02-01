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
		Client() : id(ClientID(-1)), zero_sum(0.0), is_active(true) {}
		Client(ClientID id_) {
			id = id_;
			zero_sum = 0.0;
			is_active = true;
		}

		ClientID id;
		double zero_sum;
		PlaylistVector wish_list;
		bool is_active;
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
					std::cout << "Notice: Pre-emptively suspending playback for 1s" << std::endl;
					boost::this_thread::sleep(boost::posix_time::seconds(1));
					boost::this_thread::yield();
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

		void remove_from_wish_lists(const TrackID tid, boost::mutex::scoped_lock& clients_lock) {
			BOOST_FOREACH(Client_ref c, clients) {
				for(uint32 i=0; i < c->wish_list.size(); ++i) {
					if(c->wish_list.get(i).id == tid) {
						c->wish_list.remove(i);
						--i;
					}
				}
			}
		}

		uint32 n_active_clients() {
			uint32 count = 0;
			BOOST_FOREACH(Client_ref cr, clients) {
				count += cr->is_active;
			}
			return count;
		}

		uint32 vote_min_count(TrackID tid, boost::mutex::scoped_lock& vote_min_list_lock) {
			std::map<TrackID, std::set<ClientID> >::iterator i = vote_min_list.find(tid);
			if(i != vote_min_list.end())
				return i->second.size();
			return 0;
		}

		// True if more than half of all clients voted for this song, 
		// except for the current track which requires one vote less
		// for each average_song_duration it lasts longer than
		// average_song_duration.
		bool is_down_voted(TrackID tid, boost::mutex::scoped_lock& vote_min_list_lock, boost::mutex::scoped_lock& clients_lock, boost::mutex::scoped_lock& current_track_mutex) {
			uint64 current_playtime = ac.get_current_playtime();
			uint32 vmc = vote_min_count(tid, vote_min_list_lock);
			uint32 n_avg_song_durations = (current_playtime / average_song_duration) * (current_track.id == tid);
			n_avg_song_durations = n_avg_song_durations > 0 ? n_avg_song_durations - 1 : 0;
			return (n_active_clients() - (vmc > 0) * n_avg_song_durations < 2 * vmc);
		}

		void next_song(uint32 playtime_secs) {
			{
			boost::mutex::scoped_lock clients_lock(clients_mutex);
			boost::mutex::scoped_lock current_track_lock(current_track_mutex);
			boost::mutex::scoped_lock playlist_lock(playlist_mutex);
			boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);

			cout << "Next song, playtime was " << playtime_secs << " seconds" << endl;
			bool vote_min_penalty = is_down_voted(current_track.id, vote_min_list_lock, clients_lock, current_track_lock);
			if((vote_min_penalty && playtime_secs < average_song_duration * 0.2) || (playtime_secs < 15)) {
				cout << "issueing a penalty of " << uint32(average_song_duration) << " seconds" << endl;
				playtime_secs += uint32(average_song_duration);
			}
			if(!vote_min_penalty) { // Update rolling average of last 16 songs
				average_song_duration = (average_song_duration * 15.0 + double(playtime_secs)) / 16.0;
				cout << "average song duration now at " << uint32(average_song_duration) << " seconds" << endl;
			}
			dcerr("current_track: "<<current_track.id);
			update_zero_sum(current_track.id, playtime_secs, clients_lock);
			vote_min_list.erase(current_track.id);
			remove_from_wish_lists(current_track.id, clients_lock);
			current_track = Track();
			recalculateplaylist(clients_lock, current_track_lock, vote_min_list_lock, playlist_lock);
			}
			cue_next_track();
		}

		void update_zero_sum(const TrackID target_track, const uint32 playtime_secs, boost::mutex::scoped_lock& clients_lock) {
			vector<ClientID> has_song;
			vector<ClientID> does_not_have_song;
			vector<ClientID> others;

			BOOST_FOREACH(Client_ref c, clients) {
				uint32 pos = 0;
				for (;pos < c->wish_list.size(); ++pos)
					if (c->wish_list.get(pos).id == target_track)
						break;
				if (pos != c->wish_list.size()) {
					has_song.push_back(c->id);
				}
				else {
					if((c->wish_list.size() > 0) && c->is_active) {
						// Only people who actively participate are rewarded
						// TODO: Perhaps instead of pure yes/no do some weighting
						does_not_have_song.push_back(c->id);
					}
					else {
						others.push_back(c->id);
					}
				}
			}

			double avg_min  = 0.0;
			double avg_plus = 0.0;
			if(does_not_have_song.size() > 0) {
				avg_min  = playtime_secs / double(has_song.size());
				avg_plus = playtime_secs / double(does_not_have_song.size());
			}
			if(has_song.size() == 0) { // Should never happen //TODO: Check if this can still happen
				cout << "WARNING: Division by zero, ZeroSum compromised!!" << endl;
				cout << "has_song.size()==" << has_song.size() << " n_active_clients()==" << n_active_clients() << " others.size==" << others.size() << " avg_min==" << avg_min << " avg_plus==" << avg_plus << endl;
				avg_plus = avg_min = 0.0; //FIXME : ugly hax
			}
			BOOST_FOREACH(ClientID& id, has_song) {
				double old = (*clients.find(id))->zero_sum;
				(*clients.find(id))->zero_sum -= avg_min;
				cout << "(min)  for " << STRFORMAT("%08x", id) << ": " << old << " -> " << (*clients.find(id))->zero_sum << endl;
			}
			BOOST_FOREACH(ClientID& id, does_not_have_song) {
				double old = (*clients.find(id))->zero_sum;
				(*clients.find(id))->zero_sum += avg_plus;
				cout << "(plus) for " << STRFORMAT("%08x", id) << ": " << old << " -> " << (*clients.find(id))->zero_sum << endl;
			}
			BOOST_FOREACH(ClientID& id, others) {
				cout << "(same) for " << STRFORMAT("%08x", id) << ": " << (*clients.find(id))->zero_sum << endl;
			}
		}

		boost::barrier data_source_renewed_barrier;
		void cue_next_track() {
			boost::mutex::scoped_lock lock(add_datasource_thread_mutex);
			if(add_datasource_thread.get_id() != boost::this_thread::get_id()) //FIXME: probably ac.set_data_source() should throw an exception when adding an empty datasource, instead of calling playback_finished->next_song
				add_datasource_thread.join();
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
			{ // NOTE: Since we need to start songs 'instantly' (without the playlist/wish_lists changing)
			  //       we need to do penalties here if we still want to skip as many downvoted songs
			  //       as possible each round.
				boost::mutex::scoped_lock clients_lock(clients_mutex);
				boost::mutex::scoped_lock current_track_lock(current_track_mutex);
				boost::mutex::scoped_lock playlist_lock(playlist_mutex);
				boost::mutex::scoped_lock server_datasource_lock(server_datasource_mutex);
				boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);
				bool found_track = false;
				while(!found_track && (playlist.size()>0)) {
					TrackID tid = playlist.get(0).id;
					found_track = true;
					if(is_down_voted(tid, vote_min_list_lock, clients_lock, current_track_lock)) {
						cout << "Skipping track " << tid << ", assuming a duration of " << average_song_duration << " seconds" << endl;
						// FIXME: average song duration does not have a mutex yet (does it need one?)
						update_zero_sum(tid, average_song_duration, clients_lock);
						vote_min_list.erase(tid);
						remove_from_wish_lists(tid, clients_lock);
						// TODO: This may take a while, we could optimise synced_playlist
						//       a bit do update it's internal vector immidiately, and only
						//       do the network update in the background.
						recalculateplaylist(clients_lock, current_track_lock, playlist_lock, vote_min_list_lock);
						found_track = false;
					}
				}
				playlist.sync();

				// NOTE: On occasion, something funny happens with the playlist's size,
				//       but it's been so long since I've hacked on this code that I
				//       don't know how to reproduce this any more, be warned though...
				current_track = Track();
				server_datasource.reset();
				if(playlist.size() > 0) {
					server_datasource = boost::shared_ptr<ServerDataSource>(new ServerDataSource());
					current_track = playlist.get(0);
					message_request_file_ref msg(new message_request_file(current_track.id));
					dcerr("requesting " << current_track.id);
					networkhandler.send_message(current_track.id.first, msg);
				}
				ds = server_datasource;
			}
			data_source_renewed_barrier.wait();
			ac.set_data_source(ds);
			ac.start_playback(); // FIXME: do this here?
		}

		void remove_client(ClientID id) {
			boost::mutex::scoped_lock clients_lock(clients_mutex);
			boost::mutex::scoped_lock current_track_lock(current_track_mutex);
			boost::mutex::scoped_lock playlist_lock(playlist_mutex);
			boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);

			cout << "Removing client " << STRFORMAT("%08x", id) << std::endl;

			ClientMap::iterator i = clients.find(id);
			if (i == clients.end())
				return;
			Client_ref cr = *i;

			{
				boost::mutex::scoped_lock lock(server_datasource_mutex);
				if(current_track.id.first == id && server_datasource) { // Client which is serving current file disconnected!
					server_datasource->set_wait_for_data(false);
				}
			}

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

			// check if somebody other than this client is still listening to the current track.
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
			}
			cr->is_active = false;
			if(!has_listeners && cr->wish_list.size()>0) { // Cue next song since no-one wants to hear this now
				PlaylistVector pv(cr->wish_list);
				cr->wish_list.clear();
				cr->wish_list.append(pv.get(0)); // we know there must be at least one track in this wishlist, and we know (for now) that this must the current track
				pv.remove(0);
				// NOTE:
				//  * Also calls next_song which is important since if has_song becomes empty
				//    zero_sum is compromised.
				//  * Will not try to enque a song from this client since we just removed all
				//    his songs from his wish_lists, and he's now inactive.
				//  * Will not race msg_playlist_update because this client is inactive
				//  * Might race msg_connect / msg_playlist_update combo...
				//
				// make sure we don't hold any locks when we start calling other functions
				vote_min_list_lock.unlock();
				playlist_lock.unlock();
				current_track_lock.unlock(); 
				clients_lock.unlock();
				ac.stop_playback(); // client should still be active and receive penalty
				clients_lock.lock();
				current_track_lock.lock();
				playlist_lock.lock();
				vote_min_list_lock.lock();
				cr->wish_list = pv;
			}
			cr->wish_list.clear(); // FIXME: This should be removed as soon as clients can manipulate their own playlists

			BOOST_FOREACH(Client_ref i, clients) {
				cout << "remove: " << STRFORMAT("%08x", i->id) << ": " << i->zero_sum << std::endl;
			}
			
			recalculateplaylist(clients_lock, current_track_lock, playlist_lock, vote_min_list_lock);
		}

		void handle_received_message(const messageref m, ClientID id) {
			bool recalculateplaylist = false;
			bool is_known_client = false;
			{
				boost::mutex::scoped_lock lock_clients(clients_mutex);
				ClientMap::iterator i = clients.find(id);
				is_known_client = (i != clients.end()) && ((*i)->is_active);  // Ensure that we know this client, and he is not currently disconnected
			}
			if((m->get_type() ==  message::MSG_CONNECT) || is_known_client) { // unless it is a genuine connection request.
				switch(m->get_type()) {
					case message::MSG_CONNECT: {
						dcerr("Received a MSG_CONNECT from " << STRFORMAT("%08x", id));
						{
							boost::mutex::scoped_lock lock_clients(clients_mutex);
							ClientMap::iterator i = clients.find(id);
							bool is_new_client = (i == clients.end());
							Client_ref cr = is_new_client ? Client_ref(new Client(id)) : *i;
							cr->is_active = true;
							clients.insert(cr);
							cout << (string("Client ") + (is_new_client ? "" : "re-") + STRFORMAT("connect detected for %08x, zero_sum: %i\n", id, cr->zero_sum));
							recalculateplaylist = cr->wish_list.size() > 0; 
							BOOST_FOREACH(Client_ref i, clients) {
								cout << "connect: " << STRFORMAT("%08x", i->id) << ": " << i->zero_sum << std::endl;
							}
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
							uint32 i = 0;
							while(i < pl.size()) {
								ClientMap::iterator cmi = clients.find(pl.get(i).id.first);
								if(cmi == clients.end() || !(*cmi)->is_active ) {
									// This song does not belong to any client so it is an obvious fake
									// (or someone is trying to add files from a client which has already 
									// disconnected), so remove it from the wishlist.
									// TODO: Possibly also check that the minor-id (id.second) is valid
									//       somehow (although we should get feedback from the client
									//       in question when requesting an invalid track, and more
									//       importantly: fix this properly since we still have a
									//       race-condition when a client disconnects (hard quit/crash/
									//       loss of connectivity/etc) at the same moment that we enqueue
									//       one of his tracks.
									pl.remove(i--);
								}
								++i;
							}
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
						{
						boost::mutex::scoped_lock current_track_lock(current_track_mutex);
						if(current_track.id != msg->id) break;
						}
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
							boost::mutex::scoped_lock current_track_lock(current_track_mutex);
							boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);
							vote_min_list[msg->id].insert(id);
							if((current_track.id==msg->id) && is_down_voted(msg->id, vote_min_list_lock, clients_lock, current_track_lock)) {
								boost::mutex::scoped_lock server_datasource_lock(server_datasource_mutex);
								if(server_datasource) {
									/* Notify sender to stop sending */
									vector<uint8> empty;
									message_request_file_result_ref msg(new message_request_file_result(empty, current_track.id));
									networkhandler.send_message(current_track.id.first, msg);
// 									vote_min_list.erase(current_track.id); // Don't spam sender
									clients_lock.unlock();
									current_track_lock.unlock();
									vote_min_list_lock.unlock();
									server_datasource_lock.unlock();
									ac.stop_playback(); // might call next_song (AC guarantees next_song is called only once even when EOF)
								}
							}
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
				{
					boost::mutex::scoped_lock clients_lock(clients_mutex);
					boost::mutex::scoped_lock current_track_lock(current_track_mutex);
					boost::mutex::scoped_lock playlist_lock(playlist_mutex);
					boost::mutex::scoped_lock vote_min_list_lock(vote_min_list_mutex);
					this->recalculateplaylist(clients_lock, current_track_lock, playlist_lock, vote_min_list_lock);
				}
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
				// no longer non_unique because we shouldn't need to insert multiple clients with an invalid id
				boost::multi_index::ordered_unique<boost::multi_index::member<Client, ClientID, &Client::id> >
			>
		> ClientMap;
		ClientMap clients;
		boost::mutex vote_min_list_mutex;
		std::map<TrackID, std::set<ClientID> > vote_min_list;
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
		double average_song_duration;

		struct sort_by_zero_sum_decreasing {
			bool operator()(const std::pair<Client_ref, uint32>& l, const std::pair<Client_ref, uint32>& r) const {
				return l.first->zero_sum > r.first->zero_sum;
			}
		};

		void recalculateplaylist(
			boost::mutex::scoped_lock& clients_lock,
			boost::mutex::scoped_lock& current_track_lock,
			boost::mutex::scoped_lock& vote_min_list_lock,
			boost::mutex::scoped_lock& playlist_lock
		) {
			std::vector<Track> new_playlist;
			if(current_track != Track())
				new_playlist.push_back(current_track);

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
						const Track& track(i.first->wish_list.get(i.second++));
						bool present = false;
						BOOST_FOREACH(Track& t, new_playlist) {
							present = present || (t == track);
						}
						ClientMap::iterator cmi = clients.find(track.id.first);
						if((!present) && (cmi != clients.end() && (*cmi)->is_active)) {
							new_playlist.push_back(track);
						}
					}
				}
			}

			playlist.clear();
			playlist.append(new_playlist);
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
