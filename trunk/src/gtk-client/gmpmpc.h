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
	#include <glade/glade.h>
// 	#include <gtkmm.h>
	#include "../types.h"

	#define try_connect_signal(xml_source, widget_name, signal_name) {\
		GtkWidget* widget; \
		widget = glade_xml_get_widget (xml_source, #widget_name); \
		if(widget==NULL) { \
			fprintf(stderr,"Error: can not find widget `widget_name'!\n"); \
			false; \
		} \
		else { \
			g_signal_connect (G_OBJECT (widget), #signal_name, G_CALLBACK (widget_name ## _ ## signal_name), NULL); \
			true; \
		}\
		}

	uint32 show_gui();
#endif
