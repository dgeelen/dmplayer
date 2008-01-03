//
// C++ Implementation: gmpmpc_select_server
//
// Description:
//
//
// Author:  <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "gmpmpc.h"
#include "gmpmpc_select_server.h"
#include "gmpmpc_select_server_window.glade.h"
#include "gtk-2.0/gtk/gtk.h"

GladeXML *select_server_window = NULL;

bool select_server_initialise_window() {
	/* load the interface from the xml buffer */
	select_server_window = glade_xml_new_from_buffer(gmpmpc_select_server_window_glade_definition, gmpmpc_select_server_window_glade_definition_size, NULL, NULL);
	if(select_server_window != NULL) {
		/* Have the delete event (window close) hide the windows and apply setting*/
		try_connect_signal(select_server_window, window_select_server, delete_event);
	}
	return select_server_window != NULL;
}

void window_select_server_delete_event(GtkWidget *widget, gpointer user_data) {
	if( select_server_window == NULL) {
		fprintf(stderr, "window_select_server_delete_event(): Error: select_server_window is NULL!\n");
		return;
	}
	fprintf(stderr, "window_select_server_delete_event()\n");
	GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
	gtk_widget_hide_all(wnd);
	fprintf(stderr, "window_select_server_delete_event() done\n");
}

void imagemenuitem_select_server_activate(GtkWidget *widget, gpointer user_data) {
	if( select_server_window == NULL) {
		fprintf(stderr, "imagemenuitem_select_server_activate(): Error: select_server_window is NULL!\n");
		return;
	}
	fprintf(stderr, "imagemenuitem_select_server_activate()\n");
	GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
	gtk_widget_show_all(wnd);
	fprintf(stderr, "imagemenuitem_select_server_activate() done\n");
}
