#include "mpmpd.h"
#include "network-core.h"
#include "network-handler.h"
#include <iostream>
#include <boost/program_options.hpp>

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
	
	return 0;
}
