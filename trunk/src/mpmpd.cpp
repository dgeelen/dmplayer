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

namespace po = boost::program_options;
using namespace std;

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

	const message_connect msg;
	const message* const msg1=&msg;
	message* msg2;
	stringstream ss1;
	boost::archive::text_oarchive oa(ss1);
	oa << msg1;

	stringstream ss2(ss1.str());
	boost::archive::text_iarchive ia(ss2);
	ia >> msg2;
	cout << "ss1: \"" << ss1.str() << "\"\nss2: \""<< ss2.str() << "\"\n";
//
// 	exit(0);

	dcerr("Starting network_handler");
	network_handler nh(listen_port, server_name);

	AudioController ac;;
	if(filename!= "") {
		ac.test_functie(filename);
	}

	TrackDataBase tdb;
	tdb.add_directory( musix );
	map<string, string> m;
	m["FILENAME"] = findtext;
	Track t(0, "", m);
	vector<Track> s = tdb.search(t);
	BOOST_FOREACH(Track& tr, s) {
		dcerr( tr.filename );
	}

	cout << "Press any key to quit\n";
	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
