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
#include <boost/thread/mutex.hpp>
#include "synced_playlist.h"
#include "util/StrFormat.h"
#include <algorithm>

namespace po = boost::program_options;
using namespace std;

class ServerPlaylistReceiver: public PlaylistVector {
	public:
	virtual void add(const Track& track)
	{
		PlaylistVector::add(track);
	}

	virtual void remove(uint32 pos)
	{
		PlaylistVector::remove(pos);
	}

	virtual void insert(uint32 pos, const Track& track)
	{
		PlaylistVector::insert(pos, track);
	}

	virtual void clear()
	{
		PlaylistVector::clear();
	}
};

class Client {
	public:
		Client(ClientID id_) {
			id = id_;
			zero_sum = 0.0;
		}

		~Client() {
			dcerr("shutting down");
			id = -1;
			zero_sum = -1.0;
			dcerr("shut down");
		}

		ClientID id;
		double zero_sum;
		ServerPlaylistReceiver wish_list;
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

		virtual long getpos() {
			return position;
		}

		virtual bool exhausted() {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			return (!wait_for_data) && data_exhausted;
		}

		virtual void reset() {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			position = 0;
			data_exhausted = data_buffer.size()==0;
		}

		virtual uint32 getData(uint8* buffer, uint32 len) {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			size_t n = 0;
			if (!finished)
				n = std::min(size_t(len), size_t(data_buffer.size() - position));
			if (n>0)
				memcpy(buffer, &data_buffer[position], n);
			position+=n;
			if(n==0) {
				data_exhausted = true;
				if(wait_for_data)
					boost::thread::yield();
			}
			return n;
		}

		void appendData(std::vector<uint8>& data) {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			size_t n = data_buffer.size();
			data_buffer.insert(data_buffer.end(), data.begin(), data.end());
			data_exhausted = false;
		}

		void stop() {
			finished = true;
			boost::mutex::scoped_lock lock(data_buffer_mutex);
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
		{
			networkhandler.server_message_receive_signal.connect(boost::bind(&Server::handle_received_message, this, _1, _2));
			dcerr("Started network_handler");
			server_datasource = boost::shared_ptr<ServerDataSource>((ServerDataSource*)NULL);
			add_datasource = true;
			message_loop_running = true;
			vote_min_penalty = false;
			average_song_duration = 231; // Obtained by scanning personal music library (xxx files)
			dcerr("Starting message_loop_thread");
			boost::thread tt(makeErrorHandler(boost::bind(&Server::message_loop, this)));
			message_loop_thread.swap(tt);
			ac.playback_finished.connect(boost::bind(&Server::next_song, this, _1));
			ac.start_playback();
		}

		~Server() {
			dcerr("shutting down");
			message_loop_running = false;
			dcerr("Joining message_loop_thread");
			message_loop_thread.join();
			dcerr("shut down");
		}

		void next_song(uint32 playtime_secs) {
			dcerr("Next song, playtime was " << playtime_secs << " seconds");
			if((vote_min_penalty && playtime_secs < average_song_duration * 0.2) || (playtime_secs < 15)) {
				dcerr("issueing a penalty of " << uint32(average_song_duration) << " seconds");
				playtime_secs += uint32(average_song_duration);
			}
			if(!vote_min_penalty) { // Update rolling average of last 16 songs
				average_song_duration = (average_song_duration * 15.0 + double(playtime_secs)) / 16.0;
				dcerr("average song duration now at " << uint32(average_song_duration) << " seconds");
			}
			vote_min_penalty = false;

			vector<ClientID> has_song;
			vector<ClientID> does_not_have_song;

			boost::mutex::scoped_lock lock(clients_mutex);
			Track t = currenttrack;
			BOOST_FOREACH(Client_ref c, clients) {
				int pos = 0;
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
				dcerr("(min)  for " << STRFORMAT("%08x", id) << ": " << old << " -> " << (*clients.find(id))->zero_sum);
			}
			BOOST_FOREACH( ClientID& id, does_not_have_song) {
				double old = (*clients.find(id))->zero_sum;
				(*clients.find(id))->zero_sum += avg_plus;
				dcerr("(plus) for " << STRFORMAT("%08x", id) << ": " << old << " -> " << (*clients.find(id))->zero_sum);
			}

			lock.unlock();
			recalculateplaylist();
			lock.lock();
			vote_min_list.erase(currenttrack.id);
			add_datasource = true;
		}

		void message_loop() {
			while(message_loop_running) {
				messageref m;
				{
					boost::mutex::scoped_lock lock(playlist_mutex);
					m = playlist.pop_msg();
				}
				if(m) {
					networkhandler.send_message_allclients(m);
				}
				else { // meh
					{
						boost::mutex::scoped_lock lock(playlist_mutex);
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
						boost::mutex::scoped_lock lock(clients_mutex);
						std::set<ClientID> votes = vote_min_list[currenttrack.id];
						if(server_datasource && votes.size() > 0 && (votes.size()*2) > clients.size()) {
							/* Notify sender to stop sending */
							vector<uint8> empty;
							message_request_file_result_ref msg(new message_request_file_result(empty, currenttrack.id));
							networkhandler.send_message(currenttrack.id.first, msg);

							/* Current song will incur a penalty */
							vote_min_penalty = true;

							/* Cue next song */
							server_datasource->stop();

							/* Don't spam sender */
							vote_min_list.erase(currenttrack.id);
						}
					}
					usleep(100*1000);
				}
			}
		}

		void remove_client(ClientID id) {
			boost::mutex::scoped_lock lock(clients_mutex);
			dcerr("Removing client " << STRFORMAT("%08x", id));
			if (clients.find(id) == clients.end())
				return;
			if(currenttrack.id.first == id && server_datasource) { // Client which is serving current file disconnected!
				server_datasource->set_wait_for_data(false);
			}
			Client_ref cr = *clients.find(id);
			double total = cr->zero_sum;
			dcerr("total zero sum:" << total << '\n');
			clients.erase(id);
			if(cr->wish_list.size() > 0 && currenttrack.id == cr->wish_list.get(0).id)
				server_datasource->stop();

			typedef std::pair<TrackID, std::set<ClientID> > vtype;
			BOOST_FOREACH(vtype i, vote_min_list) {
				if(i.second.find(cr->id) != i.second.end())
					vote_min_list.erase(vote_min_list.find( i.first ));
			}
			if (total == 0) return;
			BOOST_FOREACH(Client_ref i, clients) {
				double old = i->zero_sum;
				i->zero_sum += total/clients.size();
				dcerr("(dc)  for " << STRFORMAT("%08x", i->id) << ": " << old << " -> " << i->zero_sum << '\n');
			}

			/* Clear memory used by client */
// 			clients.erase(clients.find(id)); //boom segfault? Are we erasing something twice? (see a couple of lines up)
		}

		void handle_received_message(const messageref m, ClientID id) {
			switch(m->get_type()) {
				case message::MSG_CONNECT: {
					dcerr("Received a MSG_CONNECT from " << STRFORMAT("%08x", id));
					{
						boost::mutex::scoped_lock lock(clients_mutex);
						clients.insert(Client_ref(new Client(id)));
					}
					{
						boost::mutex::scoped_lock lock(playlist_mutex);
						networkhandler.send_message(id, messageref(new message_playlist_update(playlist)));
					}
				} break;
				case message::MSG_ACCEPT: {
					dcerr("Received a MSG_ACCEPT from " << STRFORMAT("%08x", id));
				}; break;
				case message::MSG_DISCONNECT: {
					dcerr("Received a MSG_DISCONNECT from " << STRFORMAT("%08x", id));
					remove_client(id);
					recalculateplaylist();
				} break;
				case message::MSG_PLAYLIST_UPDATE: {
					dcerr("Received a MSG_PLAYLIST_UPDATE from " << STRFORMAT("%08x", id));
					message_playlist_update_ref msg = boost::static_pointer_cast<message_playlist_update>(m);
					{
						boost::mutex::scoped_lock lock(clients_mutex);
						msg->apply(&(*clients.find(id))->wish_list);
					}
					recalculateplaylist();
				}; break;
				case message::MSG_QUERY_TRACKDB: {
					dcerr("Received a MSG_QUERY_TRACKDB from " << STRFORMAT("%08x", id));
				}; break;
				case message::MSG_QUERY_TRACKDB_RESULT: {
					dcerr("Received a MSG_QUERY_TRACKDB_RESULT from " << STRFORMAT("%08x", id));
					message_query_trackdb_result_ref msg = boost::static_pointer_cast<message_query_trackdb_result>(m);
					uint32 qid = msg->qid;
					std::pair<ClientID, TrackID> vote;
					std::map<uint32, std::pair<ClientID, TrackID> >::iterator finder = query_queue.find(qid);
					if (finder != query_queue.end()) {
						if(msg->result.size()==1) {
							boost::mutex::scoped_lock lock(playlist_mutex);
							playlist.add(msg->result.front());
						}
						query_queue.erase(finder);
					} else {
						dcerr("warning: ignoring query result");
					}
				}; break;
				case message::MSG_REQUEST_FILE: {
					dcerr("Received a MSG_REQUEST_FILE from " << STRFORMAT("%08x", id));
				}; break;
				case message::MSG_REQUEST_FILE_RESULT: {
					dcerr("Received a MSG_REQUEST_FILE_RESULT from " << STRFORMAT("%08x", id));
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
						vote_min_list[msg->getID()].insert(id);
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
	private:
		SyncedPlaylist playlist;
		boost::mutex playlist_mutex;
		Track currenttrack;
		network_handler networkhandler;
		AudioController ac;
		bool message_loop_running;
		std::map<uint32, std::pair<ClientID, TrackID> > query_queue;
		typedef boost::multi_index_container<
			Client_ref,
			boost::multi_index::indexed_by<
				boost::multi_index::ordered_unique<boost::multi_index::member<Client, ClientID, &Client::id> >
			>
		> ClientMap;
		ClientMap clients;
		std::map<TrackID, std::set<ClientID> > vote_min_list;
		bool vote_min_penalty;
		boost::mutex clients_mutex;

		boost::thread message_loop_thread;
		boost::shared_ptr<ServerDataSource> server_datasource;

		/* Some playback related varialbles*/
		bool add_datasource;
		double average_song_duration;

		/* note: struct sorthelper is (only) used in recalculateplaylist() */
		struct sorthelper {
			bool operator()(const std::pair<Client_ref, uint32>& l, const std::pair<Client_ref, uint32>& r) const {
				return l.first->zero_sum > r.first->zero_sum;
			}
		};

		void recalculateplaylist() {
			boost::mutex::scoped_lock locka(clients_mutex);
			boost::mutex::scoped_lock lockb(playlist_mutex);
			playlist.clear();
			std::vector<std::pair<Client_ref, uint32> > client_list;
			uint32 maxsize = 0;
			BOOST_FOREACH(Client_ref c, clients) {
				if (c->wish_list.size() > 0)
					client_list.push_back(std::pair<Client_ref, uint32>(c, 0));
				if(c->wish_list.size() > maxsize)
					maxsize = c->wish_list.size();
			}

			std::sort(client_list.begin(), client_list.end(), sorthelper() );

			bool done = false;
			while (!done) {
				done = true;
				typedef std::pair<Client_ref, uint32> vt;
				BOOST_FOREACH(vt& i, client_list) {
					if (i.second < i.first->wish_list.size()) {
						playlist.add(i.first->wish_list.get(i.second++));
						done = false;
					}
				}
			}
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
	     << "\n";

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", po::bool_switch(&showhelp)                   , "produce help message")
			("port", po::value(&listen_port)->default_value(0), "listen port for daemon (TCP part)")
			("file", po::value(&filename)->default_value("")      , "file to play (Debug for fmod lib)")
			("name", po::value(&server_name)->default_value("mpmpd V" MPMP_VERSION_STRING), "Server name")
			("musix", po::value(&musix)->default_value("")      , "directory to add music from")
			("find", po::value(&findtext)->default_value("")      , "text to find in database")
	;

	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (showhelp) {
		cout << desc << "\n";
		return 1;
	}

	if(filename != "") {
		AudioController ac;
		ac.test_functie(filename);
		cout << "Press any key to quit\n";
		getchar();
		return 0;
	}

	Server svr(listen_port, server_name);

	cout << "Press any key to quit\n";
	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
