#include "cross-platform.h"
#include "mpmpd.h"
#include "network-handler.h"
#include "error-handling.h"
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include "audio/audio_controller.h"
#include "playlist_management.h"
#include "boost/filesystem.hpp"
#include "playlist_management.h"
#include <boost/thread/mutex.hpp>

namespace po = boost::program_options;
using namespace std;

class ServerPlaylist : public IPlaylist {
	private:
		std::deque<message_playlist_update_ref> msgque;
		PlaylistVector data;
		mutable boost::mutex internal_mutex;
		//virtual void vote(TrackID id) { Playlist::vote(id); };
		//virtual void add(Track track)  { Playlist::add(track); };
		//virtual void clear()  { Playlist::clear(); };
	public:
		virtual void add(const Track& track) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(track)));
		}

		/// removes track at given position from the playlist
		virtual void remove(uint32 pos) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(pos)));
		}

		/// inserts given track at given position in the playlist
		virtual void insert(uint32 pos, const Track& track) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(track, pos)));
		}

		/// moves track from a given position to another one
		virtual void move(uint32 from, uint32 to) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(from, to)));
		}

		/// clears all tracks from the playlist
		virtual void clear() {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update()));
		}

		/// returns the number of tracks in the playlist
		virtual uint32 size() const {
			boost::mutex::scoped_lock lock(internal_mutex);
			return data.size();
		}

		/// returns the track at the given position
		virtual const Track& get(uint32 pos) const {
			boost::mutex::scoped_lock lock(internal_mutex);
			return data.get(pos);
		}

		messageref pop_msg() {
			boost::mutex::scoped_lock lock(internal_mutex);
			message_playlist_update_ref ret;
			if (!msgque.empty()) {
				ret = msgque.front();
				ret->apply(&data);
				msgque.pop_front();
			}

			return ret;
		}
};

class Server {
	public:
		Server(int listen_port, string server_name)
			: networkhandler(listen_port, server_name)
		{
			dcerr("Started network_handler");
			cqid =0;
			done = false;
			message_loop_thread = boost::thread(makeErrorHandler(boost::bind(&Server::message_loop, this)));
			networkhandler.server_message_receive_signal.connect(boost::bind(&Server::handle_received_message, this, _1, _2));
		}

		~Server() {
			done = true;
			message_loop_thread.join();
		}

		void message_loop() {
			while(!done) {
				messageref m = playlist.pop_msg();
				if(m) {
					networkhandler.send_message_allclients(m);
				}
				else {
					usleep(100);
				}
			}
		}

		void handle_received_message(const messageref m, ClientID id) {
			//playlist.q_clear();
			switch(m->get_type()) {
				case message::MSG_CONNECT: {
					dcerr("Received a MSG_CONNECT from " << id);
					networkhandler.send_message(id, messageref(new message_playlist_update(playlist)));
				} break;
				case message::MSG_ACCEPT: {
					dcerr("Received a MSG_ACCEPT from " << id);
				}; break;
				case message::MSG_DISCONNECT: {
					dcerr("Received a MSG_DISCONNECT from " << id);
				} break;
				case message::MSG_PLAYLIST_UPDATE: {
					dcerr("Received a MSG_PLAYLIST_UPDATE from " << id);
				}; break;
				case message::MSG_QUERY_TRACKDB: {
					dcerr("Received a MSG_QUERY_TRACKDB from " << id);
				}; break;
				case message::MSG_QUERY_TRACKDB_RESULT: {
					dcerr("Received a MSG_QUERY_TRACKDB_RESULT from " << id);
					message_query_trackdb_result_ref msg = boost::static_pointer_cast<message_query_trackdb_result>(m);
					uint32 qid = msg->qid;
					std::pair<ClientID, TrackID> vote;
					std::map<uint32, std::pair<ClientID, TrackID> >::iterator finder = vote_queue.find(qid);
					if (finder != vote_queue.end()) {
						if(msg->result.size()==1) {
							playlist.add(msg->result.front());
						}
						vote_queue.erase(finder);
					} else {
						dcerr("warning: ignoring query result");
					}
				}; break;
				case message::MSG_REQUEST_FILE: {
					dcerr("Received a MSG_REQUEST_FILE from " << id);
				}; break;
				case message::MSG_REQUEST_FILE_RESULT: {
					dcerr("Received a MSG_REQUEST_FILE_RESULT from " << id);
				}; break;
				case message::MSG_VOTE: {
					dcerr("Received a MSG_VOTE from " << id);
					message_vote_ref msg = boost::static_pointer_cast<message_vote>(m);
					Track query;
					query.id = msg->getID();
					vote_queue[++cqid] =  std::pair<ClientID, TrackID>(id, query.id);
					message_query_trackdb_ref sendmsg(new message_query_trackdb(cqid, query));
					dcerr("Sending to " << query.id.first);
					networkhandler.send_message(query.id.first, sendmsg);
				}; break;
				default: {
					dcerr("Ignoring unknown message of type " << m->get_type() << " from " << id);
				} break;
			}
			// 					message_connect_ref msg = boost::static_pointer_cast<message_connect>(m);
		}
ServerPlaylist playlist;//debug
	private:
// 		ServerPlaylist playlist;
		network_handler networkhandler;
		std::map<uint32, std::pair<ClientID, TrackID> > vote_queue;
		uint32 cqid;
		bool done;
		boost::thread message_loop_thread;
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
			("port", po::value(&listen_port)->default_value(12345), "listen port for daemon (TCP part)")
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

	AudioController ac;
	if(filename!= "") {
		ac.test_functie(filename);
	}

	if (musix != "") {
		TrackDataBase tdb;
		tdb.add_directory( musix );
		MetaDataMap m;
		m["FILENAME"] = findtext;
		Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), m);
		vector<LocalTrack> s = tdb.search(query);
		BOOST_FOREACH(LocalTrack& tr, s) {
			Track t(TrackID(ClientID(0),tr.id), tr.metadata );
			svr.playlist.add(t);
			svr.playlist.pop_msg();
			dcerr( tr.filename );
		}
	}

	cout << "Press any key to quit\n";
	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
