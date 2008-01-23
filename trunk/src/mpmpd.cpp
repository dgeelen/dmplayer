#include "cross-platform.h"
#include "mpmpd.h"
#include "network-handler.h"
#include "error-handling.h"
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include "audio/audio_controller.h"

namespace po = boost::program_options;
using namespace std;

// hacky! pass arguments through static vars
// this is so we can wrap xmain in the global exception handler
static int    xargc;
static char** xargv;

void xmain()
{
	int    argc = xargc;
	char** argv = xargv;

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
		throw ReturnValueException(1);
	}

	dcerr("Starting network_handler");
	network_handler nh(listen_port, server_name);

	AudioController* ac = new AudioController();
	if(filename!= "") {
		ac->test_functie(filename);
	}

	cout << "Press any key to quit\n";
	getchar();

	throw ReturnValueException(0);
}

int main(int argc, char* argv[]) {
	xargc = argc;
	xargv = argv;
	try {
		ErrorHandler(boost::bind(&xmain), true)();
	} catch (const ReturnValueException& e) {
		return e.getValue();
	}
}
