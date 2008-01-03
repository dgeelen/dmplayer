//
// C++ Interface: gmpmpc_select_server
//
// Description:
//
//
// Author:  <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GMPMPC_SELECT_SERVER_H
	#define GMPMPC_SELECT_SERVER_H
	#include <glade/glade.h>
	#include <gtkmm.h>
	#include "../types.h"
	bool select_server_initialise_window();
	void window_select_server_delete_event(GtkWidget *widget, gpointer user_data);
	void imagemenuitem_select_server_activate(GtkWidget *widget, gpointer user_data);
#endif
