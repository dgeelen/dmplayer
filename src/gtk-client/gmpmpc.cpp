#include "gmpmpc.h"
#include "gmpmpc_main_window.glade.h"
#include <glade/glade.h>
#include <gtkmm.h>

using namespace std;

int main ( int argc, char *argv[] )
{
	gtk_init (&argc, &argv);
	return show_gui();
}

/***
 * NOTE:
 * Keep the bindings/connections in this file, and put the actual
 * handler's routines in other files.
 */

GladeXML  *main_window;

uint32 show_gui() {
	GtkWidget *widget;

	/* load the interface from the xml buffer */
	main_window = glade_xml_new_from_buffer(gmpmpc_main_window_glade_definition, gmpmpc_main_window_glade_definition_size, NULL, NULL);

	/* connect the signals in the interface */
// 	widget = glade_xml_get_widget (main_window, "btnSendMessage");
// 	if(widget==NULL) {
// 		fprintf(stderr,"Error: can not find widget `btnSendMessage'!\n");
// 		return -1;
// 	}
// 	else {
// 		g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (btnSendMessage_clicked), NULL);
// 	}

	/* Have the delete event (window close) end the program */
	widget = glade_xml_get_widget (main_window, "gmpmpc_main_window");
	g_signal_connect (G_OBJECT (widget), "delete_event", G_CALLBACK (gtk_main_quit), NULL);

	/* start the event loop. TODO: Do this in the background? */
	//spawnThread(gtk_main(), NULL); ?
	gtk_main ();
	return true;
}
