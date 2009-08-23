#ifndef GMPMPC_H
	#define GMPMPC_H
	#include "gmpmpc_trackdb.h"
	#include "gmpmpc_playlist.h"
	#include "gmpmpc_select_server.h"
	#include "dispatcher_marshaller.h"
	#include "gmpmpc_connection_handling.h"
	#include "../middle_end.h"
	#include "../error-handling.h"
	#include "../network-handler.h"
	#include "../playlist_management.h"
	#include "../packet.h"
	#include <iostream>
	#include <boost/shared_ptr.hpp>
	#include <boost/signals.hpp>
	#include <boost/bind.hpp>
	#include <boost/function.hpp>
	#include <gtkmm/box.h>
	#include <gtkmm/main.h>
	#include <gtkmm/menu.h>
	#include <gtkmm/paned.h>
	#include <gtkmm/frame.h>
	#include <gtkmm/window.h>
	#include <gtkmm/treeview.h>
	#include <gtkmm/statusbar.h>
	#include <gtkmm/uimanager.h>
	#include <gtkmm/statusicon.h>
	#include <gtkmm/actiongroup.h>
	#include <gtkmm/scrolledwindow.h>

	class GtkMpmpClientWindow : public Gtk::Window {
		public:
			GtkMpmpClientWindow();
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
			Gtk::Menu*      menubar_ptr; // Menu is created dynamically using UIManager
			Gtk::Statusbar  statusbar;
			Glib::RefPtr<Gdk::Pixbuf>     statusicon_pixbuf;
			Glib::RefPtr<Gtk::StatusIcon> statusicon;
			gmpmpc_trackdb_widget*        trackdb_widget;
			gmpmpc_playlist_widget        playlist_widget;
			gmpmpc_select_server_window   select_server_window;

			/* Functions */
			void on_menu_file_preferences();
			void on_menu_file_quit();
			void on_menu_file_connect();
// 			void on_connection_accepted(ClientID id);  // callback from network connection handler
// 			void on_select_server_cancel();
			void on_playlist_update(message_playlist_update_ref m);
// 			void on_request_file(message_request_file_ref m);
// 			void on_request_file_result(message_request_file_result_ref m);
			void on_disconnect_signal(const std::string reason);
			void on_disconnect_signal_dialog_response(int response_id);
			void on_vote_signal(TrackID id, int type);
			void on_statusicon_activate();
			void on_statusicon_popup_menu(guint button, guint32 activate_time);
			void on_enqueue_track_signal(Track& track);
			bool on_delete_event(GdkEventAny* event);
			void construct_gui();
			void set_status_message(std::string msg);
			void send_message(messageref m);
			bool clear_status_messages();


			/* Variables */
// 			TrackDataBase*                                    trackdb;
// 			network_handler*                                  networkhandler;
			DispatcherMarshaller dispatcher; // Execute a function in the gui thread
			middle_end                                        middleend;
			std::map<std::string, boost::signals::connection> connected_signals;
			gmpmpc_connection_handler                         connection_handler;
			sigc::connection                                  clear_statusbar_connection;
			bool                                              is_iconified;
	};

#endif
