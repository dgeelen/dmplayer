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
			data_exhausted = false;
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
		}

		void stop() {
			finished = true;
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			position = data_buffer.size();
		}

		void set_wait_for_data(bool b) {
			boost::mutex::scoped_lock lock(data_buffer_mutex);
			data_exhausted = false;
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

			dcerr("Started network_handler");
			message_loop_running = true;
			boost::thread tt(makeErrorHandler(boost::bind(&Server::message_loop, this)));
			message_loop_thread.swap(tt);
			networkhandler.server_message_receive_signal.connect(boost::bind(&Server::handle_received_message, this, _1, _2));
			ac.playback_finished.connect(boost::bind(&Server::next_song, this, _1));
			ac.StartPlayback();
			add_datasource = true;
		}

		~Server() {
			message_loop_running = false;
			message_loop_thread.join();
		}

		void next_song(uint32 playtime_secs) {
			dcerr("Next!!! " << playtime_secs);

			vector<ClientID> has_song;
			vector<ClientID> does_not_have_song;

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

			recalculateplaylist();
			vote_min_list.erase(currenttrack.id);
			add_datasource = true;
		}

		void message_loop() {
			while(message_loop_running) {
				messageref m = playlist.pop_msg();
				if(m) {
					networkhandler.send_message_allclients(m);
				}
				else { // meh
					if(add_datasource && (playlist.size()!=0)) {
						currenttrack = playlist.get(0);
						message_request_file_ref msg(new message_request_file(currenttrack.id));
						server_datasource = boost::shared_ptr<ServerDataSource>(new ServerDataSource(*this));
						networkhandler.send_message(currenttrack.id.first, msg);
						ac.set_data_source(server_datasource);
						dcerr("requesting " << currenttrack.id.second << " from " << currenttrack.id.first);
						add_datasource = false;
					}
					std::set<ClientID> votes = vote_min_list[currenttrack.id];
					if(server_datasource && votes.size() > 0 && (votes.size()*2) >= clients.size()) {
						server_datasource->stop();
						vector<uint8> empty;
						message_request_file_result_ref msg(new message_request_file_result(empty, currenttrack.id));
						networkhandler.send_message(currenttrack.id.first, msg);
					}
					usleep(100*1000);
				}
			}
		}

		void remove_client(ClientID id) {
			if (clients.find(id) == clients.end())
				return;
			Client_ref cr = *clients.find(id);
			double total = -cr->zero_sum;
			BOOST_FOREACH(Client_ref i, clients) {
				total += i->zero_sum;
			}
			clients.erase(id);
			if(cr->wish_list.size() > 0 && currenttrack.id == cr->wish_list.get(0).id)
				server_datasource->stop();
			if (total == 0) return;
			BOOST_FOREACH(Client_ref i, clients) {
				i->zero_sum += (i->zero_sum/total)*cr->zero_sum;
			}
		}

		void handle_received_message(const messageref m, ClientID id) {
			switch(m->get_type()) {
				case message::MSG_CONNECT: {
					dcerr("Received a MSG_CONNECT from " << id);
					clients.insert(Client_ref(new Client(id)));
					networkhandler.send_message(id, messageref(new message_playlist_update(playlist)));
				} break;
				case message::MSG_ACCEPT: {
					dcerr("Received a MSG_ACCEPT from " << id);
				}; break;
				case message::MSG_DISCONNECT: {
					dcerr("Received a MSG_DISCONNECT from " << id);
					Track t = playlist.get(0);
					remove_client(id);
					recalculateplaylist();
					if(t.id.first == id) { // Client which is serving current file disconnected!
						server_datasource->set_wait_for_data(false);
					}
				} break;
				case message::MSG_PLAYLIST_UPDATE: {
					dcerr("Received a MSG_PLAYLIST_UPDATE from " << id);
					message_playlist_update_ref msg = boost::static_pointer_cast<message_playlist_update>(m);
					msg->apply(&(*clients.find(id))->wish_list);
					recalculateplaylist();
				}; break;
				case message::MSG_QUERY_TRACKDB: {
					dcerr("Received a MSG_QUERY_TRACKDB from " << id);
				}; break;
				case message::MSG_QUERY_TRACKDB_RESULT: {
					dcerr("Received a MSG_QUERY_TRACKDB_RESULT from " << id);
					message_query_trackdb_result_ref msg = boost::static_pointer_cast<message_query_trackdb_result>(m);
					uint32 qid = msg->qid;
					std::pair<ClientID, TrackID> vote;
					std::map<uint32, std::pair<ClientID, TrackID> >::iterator finder = query_queue.find(qid);
					if (finder != query_queue.end()) {
						if(msg->result.size()==1) {
							playlist.add(msg->result.front());
						}
						query_queue.erase(finder);
					} else {
						dcerr("warning: ignoring query result");
					}
				}; break;
				case message::MSG_REQUEST_FILE: {
					dcerr("Received a MSG_REQUEST_FILE from " << id);
				}; break;
				case message::MSG_REQUEST_FILE_RESULT: {
					dcerr("Received a MSG_REQUEST_FILE_RESULT from " << id);
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
					dcerr("Received a MSG_VOTE from " << id);
					message_vote_ref msg = boost::static_pointer_cast<message_vote>(m);
					if(msg->is_min_vote) {
						vote_min_list[msg->getID()].insert(id);
					}
				}; break;
				default: {
					dcerr("Ignoring unknown message of type " << m->get_type() << " from " << id);
				} break;
			}
		}
	private:
		SyncedPlaylist playlist;
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

		boost::thread message_loop_thread;
		boost::shared_ptr<ServerDataSource> server_datasource;

		bool add_datasource;

		struct sorthelper {
			bool operator()(const std::pair<Client_ref, uint32>& l, const std::pair<Client_ref, uint32>& r) const {
				return l.first->zero_sum > r.first->zero_sum;
			}
		};

		void recalculateplaylist() {
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

	Server svr(listen_port, server_name);

// 	if(filename!= "") {
// 		ac.test_functie(filename);
// 	}

// 	if (musix != "") {
// 		TrackDataBase tdb;
// 		tdb.add_directory( musix );
// 		MetaDataMap m;
// 		m["FILENAME"] = findtext;
// 		Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), m);
// 		vector<LocalTrack> s = tdb.search(query);
// 		BOOST_FOREACH(LocalTrack& tr, s) {
// 			Track t(TrackID(ClientID(0),tr.id), tr.metadata );
// 			svr.playlist.add(t);
// 			svr.playlist.pop_msg();
// 			dcerr( tr.filename );
// 		}
// 	}

	cout << "Press any key to quit\n";
	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
