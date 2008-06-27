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
	#include "../packet.h"
	#include <iostream>
	#include <boost/shared_ptr.hpp>
	#include <boost/signals.hpp>
	#include <glibmm/dispatcher.h>
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
			Gtk::Statusbar statusbar;
			gmpmpc_trackdb_widget* trackdb_widget;
			gmpmpc_playlist_widget playlist_widget;
			gmpmpc_select_server_window select_server_window;

			/* Functions */
			void on_menu_file_preferences();
			void on_menu_file_quit();
			void on_menu_file_connect();
			void on_connection_accepted(ClientID id);  // callback from network connection handler
			void on_select_server_connect(ipv4_socket_addr addr);
			void on_select_server_cancel();
			void on_playlist_update(message_playlist_update_ref m);
			void on_request_file(message_request_file_ref m);
			void on_request_file_result(message_request_file_result_ref m);
			void on_vote_signal(TrackID id, int type);
			void construct_gui();
			void set_status_message(std::string msg);
			void send_message(messageref m);
			bool clear_status_messages();


			/* Variables */
			TrackDataBase* trackdb;
			network_handler* networkhandler;
			std::map<std::string, boost::signals::connection> connected_signals;
			gmpmpc_connection_handler connection_handler;
			sigc::connection clear_statusbar_connection;

			/* Threading crap */
			//on_connection_accepted
			sigc::connection _on_connection_accepted_dispatcher_connection;
			Glib::Dispatcher _on_connection_accepted_dispatcher;
			void _on_connection_accepted();            // actually executes callback in GUI thread
			ClientID clientid;
			boost::mutex clientid_mutex;

			//on_request_file
			boost::mutex _on_request_file_threads_mutex;
			std::map<TrackID, std::pair<boost::shared_ptr<boost::thread>, boost::shared_ptr<bool> > > _on_request_file_threads;
			void _on_request_file(message_request_file_ref m, boost::shared_ptr<bool> done);

			//on_playlist_update
			sigc::connection _on_playlist_update_dispatcher_connection;
			Glib::Dispatcher _on_playlist_update_dispatcher;
			void _on_playlist_update();            // actually executes callback in GUI thread
			std::list<message_playlist_update_ref> _on_playlist_update_messages;
			boost::mutex _on_playlist_update_mutex;
	};

#endif
