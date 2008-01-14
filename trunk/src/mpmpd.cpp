#include "cross-platform.h"
#include "mpmpd.h"
#include "network-handler.h"
#include "error-handling.h"
// #include "audio/mp3/mp3_interface.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace std;
int main(int argc, char* argv[]) {
	try {
	int listen_port;
	string filename;
	string server_name;
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
	;

	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (showhelp) {
		cout << desc << "\n";
		return 1;
	}

	dcerr("Starting network_handler");
	network_handler nh(listen_port, server_name);
// 	dcerr("Starting mp3_handler");
// 	mp3_handler* handler = new mp3_handler();

/*	if (filename != "") {
		handler->Load(filename.c_str());
		handler->Play();
		std::cerr << " Now playing: " << filename << " \\o\\ \\o/ /o/" << std::endl;
	}*/
	cout << "Press any key to quit\n";
	getchar();

	return 0;
	}
	catch(char const* error_msg) {
		cout << "---<ERROR REPORT>---\n"
		     << error_msg << "\n"
		     << "---</ERROR REPORT>---\n";
	}
/*	catch(...) {
		cout << "---<ERROR REPORT>---\n"
		     << "Unknown error ?!\n"
		     << "---</ERROR REPORT>---\n";
	}*/
}
