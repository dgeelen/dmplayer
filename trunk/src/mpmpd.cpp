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
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include "audio/audio_controller.h"
#include "playlist_management.h"
#include "boost/filesystem.hpp"
#include "playlist_management.h"
#include <boost/thread/recursive_mutex.hpp>
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

		~Client() {
			id = -1;
			zero_sum = -1.0;
		}

		ClientID id;
		double zero_sum;
		PlaylistVector wish_list;
};
typedef boost::shared_ptr<Client> Client_ref;

class Server;
class ServerDataSource : public IDataSource {
	public:
		ServerDataSource(Server& _server) : server(_server) {
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
		Server& server;
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
			, message_loop_connection(networkhandler.server_message_receive_signal.connect(boost::bind(&Server::handle_received_message, this, _1, _2)))
		{
			server_datasource = boost::shared_ptr<ServerDataSource>((ServerDataSource*)NULL);
			add_datasource = true;
			message_loop_running = true;
			vote_min_penalty = false;
			// The following constant was obtained by scanning my personal music library,
			// counting only songs with a duration greater than 00:01:30 and less than
			// 00:10:00. This yielded a total duration of 3 days, 14 hours, 14 minutes and 
			// 2 seconds, or 310442 seconds, for a total of 1461 songs. Thus on average
			// a song lasted 212.486~ seconds, or just over 3.5 minutes. For esthetic reasons
			// I thus chose 210 (exactly 3.5 minutes) as a safe default.
			average_song_duration = 210;
			dcerr("Starting message_loop_thread");
			boost::thread tt(makeErrorHandler(boost::bind(&Server::message_loop, this)));
			message_loop_thread.swap(tt);
			ac.playback_finished.connect(boost::bind(&Server::next_song, this, _1));
			ac.start_playback();
			#ifdef DEBUG
				next_query_id = 0; // Intentionally not initialized.
			#endif
			networkhandler.start();
			dcerr("Started network_handler");
		}

		~Server() {
			dcerr("shutting down");
			message_loop_running = false;
			dcerr("stopping networkhandler");
			networkhandler.send_message_allclients(messageref(new message_disconnect("Server is shutting down.")));
			networkhandler.stop();
			dcerr("Joining message_loop_thread");
			message_loop_connection.disconnect();
			message_loop_thread.join();
			dcerr("shut down");
			ac.stop_playback();
			ac.playback_finished.disconnect(boost::bind(&Server::next_song, this, _1));
		}

		void next_song(uint64 playtime_secs) {
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

			vector<ClientID> has_song;
			vector<ClientID> does_not_have_song;

			boost::recursive_mutex::scoped_lock lock(clients_mutex);
			Track t = currenttrack;
			BOOST_FOREACH(Client_ref c, clients) {
				uint32 pos = 0;
				for (;pos < c->wish_list.size(); ++pos)
					if (c->wish_list.get(pos).id == t.id)
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

			lock.unlock();
			recalculateplaylist();
			lock.lock();
			{
				boost::mutex::scoped_lock lock(vote_min_list_mutex);
				vote_min_list.erase(currenttrack.id);
			}
			add_datasource = true;
		}

		void message_loop() {
			while(message_loop_running) {
				messageref m;
				{
					boost::recursive_mutex::scoped_lock lock(playlist_mutex);
					m = playlist.pop_msg();
				}
				if(m) {
					networkhandler.send_message_allclients(m);
				}
				else { // meh
					{
						boost::recursive_mutex::scoped_lock lock(playlist_mutex);
						if(add_datasource && (playlist.size()!=0)) {
							currenttrack = playlist.get(0);
							lock.unlock();
							message_request_file_ref msg(new message_request_file(currenttrack.id));
							server_datasource = boost::shared_ptr<ServerDataSource>(new ServerDataSource(*this));
							dcerr("requesting " << STRFORMAT("%08x:%08x", currenttrack.id.first, currenttrack.id.second));
							add_datasource = false;
							networkhandler.send_message(currenttrack.id.first, msg);
							/* FIXME: Possible DoS exploit:
							 * enqueue a file, then when asked for data never provide it.
							 * Will halt communication with server indefinitely
							 */
							ac.set_data_source(server_datasource);
							ac.start_playback();
						}
					}
					{
						boost::recursive_mutex::scoped_lock lock(clients_mutex);
						std::set<ClientID> votes;
						{
							boost::mutex::scoped_lock lock(vote_min_list_mutex);
							votes = vote_min_list[currenttrack.id];
						}
						if(server_datasource && votes.size() > 0 && (votes.size()*2) > clients.size()) {
							/* Notify sender to stop sending */ //FIXME: Does not seem to be honoured by gmpmpc?
							vector<uint8> empty;
							message_request_file_result_ref msg(new message_request_file_result(empty, currenttrack.id));
							networkhandler.send_message(currenttrack.id.first, msg);

							/* Current song will incur a penalty */
							vote_min_penalty = true;

							/* Cue next song */
							ac.stop_playback();

							/* Don't spam sender */
							{
							boost::mutex::scoped_lock lock(vote_min_list_mutex);
							vote_min_list.erase(currenttrack.id);
							}
						}
					}
					usleep(100*1000);
				}
			}
		}

		void remove_client(ClientID id) {
			boost::recursive_mutex::scoped_lock clock(clients_mutex);
			boost::recursive_mutex::scoped_lock pllock(playlist_mutex);
			dcerr("Removing client " << STRFORMAT("%08x", id));
			if (clients.find(id) == clients.end())
				return;
			if(currenttrack.id.first == id && server_datasource) { // Client which is serving current file disconnected!
				server_datasource->set_wait_for_data(false);
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
						if(c->wish_list.get(i).id == currenttrack.id) {
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
			if(!has_listeners) {
				// Cue next song, also calls next_song which is important since
				// if has_song becomes empty zero_sum is compromised.
				ac.stop_playback();
			}

			double total = cr->zero_sum;
			dcerr("This clients zero_sum was:" << total << '\n');
			clients.erase(id);
			BOOST_FOREACH(Client_ref i, clients) {
				double old = i->zero_sum;
				i->zero_sum += total/clients.size();
				dcerr("(dc)  for " << STRFORMAT("%08x", i->id) << ": " << old << " -> " << i->zero_sum << '\n');
			}
			recalculateplaylist();
		}

		void handle_received_message(const messageref m, ClientID id) {
			bool recalculateplaylist = false;
			{
			boost::recursive_mutex::scoped_lock lock_clients(clients_mutex);
			boost::recursive_mutex::scoped_lock lock_playlist(playlist_mutex);
			ClientMap::iterator cmi = clients.find(id);                              // Ensure that we know this client,
			if((m->get_type() ==  message::MSG_CONNECT) || (cmi != clients.end())) { // unless it is a genuine request.
				switch(m->get_type()) {
					case message::MSG_CONNECT: {
						dcerr("Received a MSG_CONNECT from " << STRFORMAT("%08x", id));
						clients.insert(Client_ref(new Client(id)));
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
						PlaylistVector& pl = ((*cmi)->wish_list);
						if((pl.size() < 100)
							|| (msg->get_type()!=message_playlist_update::UPDATE_APPEND_ONE)
							|| (msg->get_type()!=message_playlist_update::UPDATE_APPEND_MANY)
							|| (msg->get_type()!=message_playlist_update::UPDATE_INSERT_ONE)
							|| (msg->get_type()!=message_playlist_update::UPDATE_INSERT_MANY)) {
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
						if(currenttrack.id != msg->id) break;
						if(msg->data.size()) {
							server_datasource->appendData(msg->data);
						}
						else { // This was the last message, we have received the full file.
							server_datasource->set_wait_for_data(false);
						}
					}; break;
					case message::MSG_VOTE: {
						dcerr("Received a MSG_VOTE from " << STRFORMAT("%08x", id));
						message_vote_ref msg = boost::static_pointer_cast<message_vote>(m);
						if(msg->is_min_vote) {
							boost::mutex::scoped_lock lock(vote_min_list_mutex);
							vote_min_list[msg->id].insert(id);
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
			} // release locks on playlist and clientlist
			if(recalculateplaylist) {
				this->recalculateplaylist();
			}
		}
	private:
		SyncedPlaylist playlist;
		boost::recursive_mutex playlist_mutex;
		Track currenttrack;
		network_handler networkhandler;
		AudioController ac;
		bool message_loop_running;
		typedef boost::multi_index_container<
			Client_ref,
			boost::multi_index::indexed_by<
				boost::multi_index::ordered_unique<boost::multi_index::member<Client, ClientID, &Client::id> >
			>
		> ClientMap;
		ClientMap clients;
		boost::mutex vote_min_list_mutex;
		std::map<TrackID, std::set<ClientID> > vote_min_list;
		bool vote_min_penalty;
		boost::recursive_mutex clients_mutex;

		boost::thread message_loop_thread;
		boost::signals::connection message_loop_connection;
		boost::shared_ptr<ServerDataSource> server_datasource;

		boost::mutex outstanding_query_result_list_mutex;
		uint32 next_query_id;
		std::map<uint32, std::pair<std::pair<ClientID, uint32>,  std::set<ClientID> > > outstanding_query_result_list;

		/* Some playback related variables*/
		bool add_datasource;
		double average_song_duration;

		struct track_priority_properties {
			int    number_of_occurrences; // How many people have this track in their wishlist
			float  avg_wishlist_position; // What is the average index of this track in the wishlist's of those people
			double total_zero_sum;        // What is the total zero_sum of those people
			Track  track;                 // The track to which these properties apply
			bool operator<(const track_priority_properties& that) const {
				return (this->number_of_occurrences <  that.number_of_occurrences)  ||
				      ((this->number_of_occurrences == that.number_of_occurrences)  &&
					   (this->avg_wishlist_position >  that.avg_wishlist_position)) ||
					  ((this->avg_wishlist_position == that.avg_wishlist_position)  &&
					   (this->total_zero_sum        <  that.total_zero_sum));
			};
		};

		struct sort_by_zero_sum_increasing {
			bool operator()(const Client& l, const Client& r) const {
				return l.zero_sum < r.zero_sum;
			}
			bool operator()(const Client_ref& l, const Client_ref& r) const {
				return l->zero_sum < r->zero_sum;
			}
		};

		void recalculateplaylist() {
			boost::recursive_mutex::scoped_lock locka(clients_mutex);
			boost::recursive_mutex::scoped_lock lockb(playlist_mutex);

			// Sort clients by zero_sum
			// FIXME: Figure out how to do this to clients (the boost::multi_index_container)
			std::vector<Client> _clients;
			BOOST_FOREACH(Client_ref cr, clients) {
				_clients.push_back(*cr);
			}
			std::sort(_clients.begin(), _clients.end(), sort_by_zero_sum_increasing());

			std::vector<track_priority_properties> shared_tracks;
			std::vector<std::vector<Track> >  custom_tracks(_clients.size());
			int current_client = 0;
			BOOST_FOREACH(Client& c, _clients) { // NOTE: Destroys contents of _clients
				int track_position = 0;
				BOOST_FOREACH(Track& track, c.wish_list) {
					track_priority_properties tpp;
					tpp.number_of_occurrences = 1;
					tpp.avg_wishlist_position = (float)track_position;
					tpp.total_zero_sum        = c.zero_sum;
					tpp.track                 = track;
					BOOST_FOREACH(Client& oc, _clients) {
						int otrack_position = 0;
						if(oc.id != c.id) {
							BOOST_FOREACH(Track& otrack, oc.wish_list) {
								if(otrack == track) {								
									oc.wish_list.remove(otrack_position);
									tpp.number_of_occurrences += 1;
									tpp.avg_wishlist_position += otrack_position;
									tpp.total_zero_sum        += oc.zero_sum;
									break;
								}
								otrack_position++;
							}
						}
					}
					if(tpp.number_of_occurrences > 1) {
						tpp.avg_wishlist_position /= (float)tpp.number_of_occurrences;
						shared_tracks.push_back(tpp);
					}
					else {
						// NOTE: we are processing clients in order of increasing zero_sum
						//       so the clients are ordered in that order in custom_tracks too.
						custom_tracks[current_client].push_back(track);
					}
					track_position++;
				}				
				current_client++;
			}
			std::sort(shared_tracks.begin(), shared_tracks.end());

			std::vector<Track> new_playlist;
			BOOST_REVERSE_FOREACH(track_priority_properties& tpp, shared_tracks) {
				new_playlist.push_back(tpp.track);
			}
			bool done = false;
			unsigned int index = 0;
			while (!done) {
				done = true;
				BOOST_REVERSE_FOREACH(std::vector<Track>& v, custom_tracks) {
					if(v.size() > index) {
						done = false;
						new_playlist.push_back(v[index]);
					}
				}
				++index;
			}

			// Finally ensure that currenttrack is always on top
			for(unsigned int i = 0; i < new_playlist.size(); ++i) {
				if(new_playlist[i] == currenttrack) {
					new_playlist.erase(new_playlist.begin() + i);
					new_playlist.insert(new_playlist.begin(), currenttrack);
				}
			}

			playlist.clear();
			playlist.append(new_playlist);
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
