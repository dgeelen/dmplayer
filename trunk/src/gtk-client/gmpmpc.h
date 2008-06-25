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
	#include "gmpmpc_trackdb.h"
	#include "gmpmpc_select_server.h"
	#include "gmpmpc_connection_handling.h"
	#include "gmpmpc_playlist.h"
	#include "../error-handling.h"
	#include "../network-handler.h"
	#include "../playlist_management.h"
	#include <iostream>
	#include <boost/shared_ptr.hpp>
	#include <boost/signals.hpp>
	#include <gtkmm/box.h>
	#include <gtkmm/main.h>
	#include <gtkmm/menu.h>
	#include <gtkmm/paned.h>
	#include <gtkmm/frame.h>
	#include <gtkmm/window.h>
	#include <gtkmm/treeview.h>
	#include <gtkmm/statusbar.h>
	#include <gtkmm/uimanager.h>
	#include <gtkmm/actiongroup.h>
	#include <gtkmm/scrolledwindow.h>

	class GtkMpmpClientWindow : public Gtk::Window {
		public:
			GtkMpmpClientWindow(network_handler* nh, TrackDataBase* tdb);
			~GtkMpmpClientWindow();

		private:
			/* Helpers */
			Glib::RefPtr<Gtk::UIManager> m_refUIManager;
  		Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
			/* Containers (widgets not referenced by code) */
			Gtk::VBox             main_vbox;
			Gtk::HPaned           main_paned;
			Gtk::Frame            playlist_frame;
			Gtk::ScrolledWindow   playlist_scrolledwindow;
			/* Widgets (referenced by code) */
			Gtk::Menu*     menubar_ptr; // Menu is created dynamically using UIManager
// 			Gtk::TreeView  playlist_treeview;
			Gtk::Statusbar statusbar;
			gmpmpc_trackdb_widget* trackdb_widget;
			gmpmpc_playlist_widget playlist_widget;
			gmpmpc_select_server_window select_server_window;

			/* Functions */
			void on_menu_file_preferences();
			void on_menu_file_quit();
			void on_menu_file_connect();
			void on_connection_accepted();
			void on_select_server_connect(ipv4_socket_addr addr);
			void on_select_server_cancel();
			void construct_gui();
			void set_status_message(std::string msg);
			bool clear_status_messages();

			/* Variables */
			TrackDataBase* trackdb;
			network_handler* networkhandler;
			std::map<std::string, boost::signals::connection> connected_signals;
			gmpmpc_connection_handler connection_handler;
			sigc::connection clear_statusbar_connection;
	};

#endif
