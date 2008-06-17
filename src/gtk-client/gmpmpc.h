//
// C++ Interface: gmpmpc
//
// Description:
//
//
// Author:  <Da Fox>, (C) 2007,2008,2009,...
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GMPMPC_H
	#define GMPMPC_H
	#include "gtk/gtk.h"
	#include <glade/glade.h>
	#include <iostream>
	#include "gmpmpc_main_window.glade.h"
	#include "gmpmpc_select_server_window.glade.h"
	#include "../types.h"
	#include "../network-handler.h"
	#include "../error-handling.h"

	extern network_handler* gmpmpc_network_handler; //FIXME: Global variables == 3vil

	#define try_connect_signal(xml_source, widget_name, signal_name, ... ) {\
		GtkWidget* widget; \
		widget = glade_xml_get_widget (xml_source, #widget_name); \
		if(widget==NULL) { \
			std::cerr << "Error: can not find widget `" #widget_name "'!\n"; \
			false; \
		} \
		else { \
			widget_name ## _ ## signal_name ## _signal_id = g_signal_connect (G_OBJECT (widget), __VA_ARGS__ #signal_name, G_CALLBACK (widget_name ## _ ## signal_name), NULL); \
			true; \
		}\
	}

	#define disconnect_signal(xml_source, widget_name, signal_name) {\
		GtkWidget* widget; \
		widget = glade_xml_get_widget (xml_source, #widget_name); \
		if(widget==NULL) { \
			std::cerr << "Error: can not find widget `" #widget_name "'!\n"; \
			false; \
		} \
		else { \
			g_signal_handler_disconnect(G_OBJECT(widget), widget_name ## _ ## signal_name ## _signal_id);\
			true; \
		}\
	}

	#define DISPLAY_TREEVIEW_COLUMN(treeview, title, rendermode, column) { \
		GtkTreeViewColumn   *col; \
		GtkCellRenderer     *renderer; \
		col = gtk_tree_view_column_new(); \
		renderer = gtk_cell_renderer_text_new(); \
		gtk_tree_view_column_set_title(col, title); \
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col); \
		gtk_tree_view_column_pack_start(col, renderer, TRUE); \
		gtk_tree_view_column_add_attribute(col, renderer, #rendermode, column);\
	}

	#define try_with_widget(xml_source, widget_name, widget) \
	for( GtkWidget* widget = glade_xml_get_widget (xml_source, #widget_name); \
	     (widget==NULL)?(std::cerr << "Error: can not find widget `" #widget_name "'!\n", false):(widget!=(GtkWidget*)-1); widget=(GtkWidget*)-1)

	uint32 show_gui();
#endif
