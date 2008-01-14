#include "gmpmpc.h"
#include "../types.h"
#include "../network-handler.h"
#include "gmpmpc_main_window.glade.h"
#include "gmpmpc_select_server.h"

using namespace std;

network_handler nh(5844);

void server_lister_tmp( const vector<server_info>& si) {
	cout << "---SERVER_LISTING_CHANGED:";
	for(vector<server_info>::const_iterator i = si.begin(); i != si.end(); ++i ) {
		cout << "   " << *i;
	}
	cout << "+++SERVER_LISTING_CHANGED";
}

int main ( int argc, char *argv[] )
{
	fprintf(stderr, "Starting GUI..\n");
	gtk_init (&argc, &argv);
	glade_init();

	//TODO: Put gui in a thread

// 	while(1) {
// 		nh.get_available_servers();
// 		return 0;
// 	}


	uint32 retval = show_gui();
	fprintf(stderr, "Terminating GUI (%i)\n", retval);
	return retval;
}

/***
 * NOTE:
 * Keep the bindings/connections in this file, and put the actual
 * handler's routines in other files.
 */

uint32 show_gui() {
	/* load the interface from the xml buffer */
	GladeXML *main_window = glade_xml_new_from_buffer(gmpmpc_main_window_glade_definition, gmpmpc_main_window_glade_definition_size, NULL, NULL);
	if(!select_server_initialise_window()) {
		fprintf(stderr, "Error while loading server select window!\n");
		return false;
	}

	/* connect the signals in the interface */
	try_connect_signal(main_window, imagemenuitem_select_server, activate);

	/* Have the delete event (window close) end the program */
	GtkWidget* widget = glade_xml_get_widget (main_window, "gmpmpc_main_window");
	g_signal_connect (G_OBJECT (widget), "delete_event", G_CALLBACK (gtk_main_quit), NULL);

	/* start the event loop. TODO: Do this in the background? */
	//spawnThread(gtk_main(), NULL); ?
	gtk_main ();
	return true;
}
