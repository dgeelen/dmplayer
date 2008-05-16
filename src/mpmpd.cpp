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

namespace po = boost::program_options;
using namespace std;

class Server {
	public:
		Server(int listen_port, string server_name) {
		dcerr("Starting network_handler");
		networkhandler = boost::shared_ptr<network_handler>(new network_handler(listen_port, server_name));
		networkhandler->message_receive_signal_with_id.connect(boost::bind(&Server::handle_received_message, this, _1, _2));
		}

		void handle_received_message(const messageref m, ClientID id) {
			switch(m->get_type()) {
				case message::MSG_CONNECT: {
					dcerr("Received a MSG_CONNECT from " << id);
					networkhandler->send_message(id, messageref(new message_playlist_update(playlist)));
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
				}; break;
				case message::MSG_REQUEST_FILE: {
					dcerr("Received a MSG_REQUEST_FILE from " << id);
				}; break;
				case message::MSG_REQUEST_FILE_RESULT: {
					dcerr("Received a MSG_REQUEST_FILE_RESULT from " << id);
				}; break;
				default: {
					dcerr("Ignoring unknown message of type " << m->get_type() << " from " << id);
				} break;
			}
			// 					message_connect_ref msg = boost::static_pointer_cast<message_connect>(m);
		}

	private:
		Playlist playlist;
		boost::shared_ptr<network_handler> networkhandler;
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

	TrackDataBase tdb;
	tdb.add_directory( musix );
	map<string, string> m;
	m["FILENAME"] = findtext;
	vector<LocalTrack> s = tdb.search(m);
	BOOST_FOREACH(LocalTrack& tr, s) {
		dcerr( tr.filename );
	}

	cout << "Press any key to quit\n";
	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
