#include "mpmpd.h"
#include "network-core.h"
#include "network-handler.h"
#include <iostream>
#include <boost/program_options.hpp>
#include "audio/mp3/mp3_interface.h"

namespace po = boost::program_options;
using namespace std;
int main(int argc, char* argv[]) {
	int listen_port = 55555;
	cout << "starting mpmpd V" MPMP_VERSION_STRING "\n";

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", "produce help message")
			("port", po::value<int>(), "listen port for daemon")
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
	mp3_handler* handler = new mp3_handler();

	handler->Load("Nirvana - Smells Like Teen Spirit (1).mp3");
	handler->Play();
	std::cerr << " Now playing: Nirvana - Smells Like Teen Spirit \\o\\ \\o/ /o/" << std::endl;
	getchar();


	return 0;
}
