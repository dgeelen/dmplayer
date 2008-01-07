#include "cross-platform.h"
#include "mpmpd.h"
#include "network-handler.h"
#include <boost/program_options.hpp>
#include "audio/mp3/mp3_interface.h"

namespace po = boost::program_options;
using namespace std;
int main(int argc, char* argv[]) {
	int listen_port = 55555;
	cout << "starting mpmpd V" MPMP_VERSION_STRING
	#ifdef DEBUG
	     << "   [Debug Build]"
	#endif
	     << "\n";


	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", "produce help message")
			("port", po::value<int>(), "listen port for daemon")
			("file", po::value<string>(), "file to play (Debug for fmod lib)")
	;

	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		return 1;
	}
	if (vm.count("port")) {
		listen_port =  vm["port"].as<int>();
	}

	network_handler nh(listen_port);
	nh.start();
	mp3_handler* handler = new mp3_handler();

	if(vm.count("file")) {
		const char* fname = vm["file"].as<string>().c_str();
		handler->Load(fname);
		handler->Play();
		std::cerr << " Now playing: " << fname << " \\o\\ \\o/ /o/" << std::endl;
		getchar();
	}

	return 0;
}
