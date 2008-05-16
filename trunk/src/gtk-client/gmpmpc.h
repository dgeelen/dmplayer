//
// C++ Interface: gmpmpc
//
// Description:
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GMPMPC_H
	#define GMPMPC_H
	#include "../types.h"
	#include "gtk/gtk.h"
	#include <glade/glade.h>
	#include "../network-handler.h"

	extern network_handler* gmpmpc_network_handler; //FIXME: Global variables == 3vil

	#define try_connect_signal(xml_source, widget_name, signal_name) {\
		GtkWidget* widget; \
		widget = glade_xml_get_widget (xml_source, #widget_name); \
		if(widget==NULL) { \
			cerr << "Error: can not find widget `" #widget_name "'!\n"; \
			false; \
		} \
		else { \
			widget_name ## _ ## signal_name ## _signal_id = g_signal_connect (G_OBJECT (widget), #signal_name, G_CALLBACK (widget_name ## _ ## signal_name), NULL); \
			true; \
		}\
	}

	#define disconnect_signal(xml_source, widget_name, signal_name) {\
		GtkWidget* widget; \
		widget = glade_xml_get_widget (xml_source, #widget_name); \
		if(widget==NULL) { \
			cerr << "Error: can not find widget `" #widget_name "'!\n"; \
			false; \
		} \
		else { \
			g_signal_handler_disconnect(G_OBJECT(widget), widget_name ## _ ## signal_name ## _signal_id);\
			true; \
		}\
	}

	#define try_with_widget(xml_source, widget_name, widget) \
	for( GtkWidget* widget = glade_xml_get_widget (xml_source, #widget_name); \
	     (widget==NULL)?(cerr << "Error: can not find widget `" #widget_name "'!\n", false):(widget!=(GtkWidget*)-1); widget=(GtkWidget*)-1)

	uint32 show_gui();
#endif
