#define __INIT_VARS__
#include "gmpmpc.h"
#include "../types.h"
#include "../network-handler.h"
#include "../error-handling.h"
#include "gmpmpc_select_server.h"
#include "../audio/audio_controller.h"
#include <boost/program_options.hpp>
#include "gmpmpc_playlist.h"

using namespace std;
namespace po = boost::program_options;
network_handler* gmpmpc_network_handler; //FIXME: Global variables == 3vil
bool connected_to_server = false;
TreeviewPlaylist* treeview_playlist;

/* Signals */
int gmpmpc_main_window_delete_event_signal_id      = 0;
int imagemenuitem_select_server_activate_signal_id = 0;

GladeXML* main_window = NULL;

void gmpmpc_main_window_delete_event(GtkWidget *widget, gpointer user_data) {
	/* Clean up */
	select_server_uninitialise_window();
	disconnect_signal(main_window, imagemenuitem_select_server, activate);
	disconnect_signal(main_window, gmpmpc_main_window, delete_event);
	gtk_main_quit();
}

int main ( int argc, char *argv[] )
{
	int listen_port;
	bool showhelp;
	string filename;
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", po::bool_switch(&showhelp)                   , "produce help message")
			("port", po::value(&listen_port)->default_value(12345), "TCP Port")
			("file", po::value(&filename)->default_value("")      , "Filename to test with")
	;
	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (showhelp) {
		cout << desc << "\n";
		return 1;
	}

	network_handler nh(listen_port);
	AudioController ac;
	gmpmpc_network_handler = &nh;

	gtk_init (&argc, &argv);
	glade_init();

	main_window = glade_xml_new_from_buffer(gmpmpc_main_window_glade_definition,
	                                        gmpmpc_main_window_glade_definition_size,
	                                        NULL,
	                                        NULL
	                                       );
	if(!select_server_initialise_window()) {
		cerr << "Error while loading server select window!\n";
	}
	if(!playlist_initialize()) {
		cerr << "Error while initializing playlist!\n";
	}

	/* connect the signals in the interface */
	try_connect_signal(main_window, imagemenuitem_select_server, activate);
	try_connect_signal(main_window, gmpmpc_main_window, delete_event);
	if(!connected_to_server) imagemenuitem_select_server_activate(NULL, NULL);
	try_with_widget(main_window, treeview_playlist, playlist) {
		treeview_playlist = new TreeviewPlaylist((GtkTreeView*)playlist);
	}

	gtk_main ();
	return 0;
}
