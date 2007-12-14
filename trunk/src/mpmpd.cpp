#include "mpmpd.h"
#include "network-core.h"
#include "network-handler.h"
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace std;
int main(int argc, char* argv[]) {
	int listen_port = 55555;
	ipv4_addr listen_addr;
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
	cout << "Listening on port #"<< listen_port<<"\n";
	tcp_listen_socket* listener = new tcp_listen_socket(listen_addr, listen_port);
	return 0;
}
