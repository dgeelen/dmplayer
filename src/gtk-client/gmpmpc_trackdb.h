#ifndef GMPMPC_TRACKDB_H
	#define GMPMPC_TRACKDB_H
	#include "../error-handling.h"
	#include "../network-handler.h"
	#include "dispatcher_marshaller.h"
	#include "../playlist_management.h"
	#include "../middle_end.h"
	#include "track_treeview.h"
	#include <boost/signal.hpp>
	#include <gtkmm/box.h>
	#include <gtkmm/frame.h>
	#include <gtkmm/label.h>
	#include <gtkmm/entry.h>
	#include <gtkmm/widget.h>
	#include <gtkmm/button.h>
	#include <gtkmm/treeview.h>
	#include <gtkmm/scrolledwindow.h>
	#include <boost/thread/mutex.hpp>

	class gmpmpc_trackdb_widget : public Gtk::Frame {
		public:
			gmpmpc_trackdb_widget(middle_end& m);
			~gmpmpc_trackdb_widget();
			void set_clientid(ClientID id);
			boost::signal<void(Track&)>      enqueue_track_signal;
			boost::signal<void(std::string)> status_message_signal;
		private:
// 			bool focus_add_to_wishlist_button();
// 			bool on_treeview_clicked(GdkEventButton *event);
			void on_search_entry_changed();
			void on_add_to_wishlist_button_clicked();
			void on_drag_data_received_signal(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const
			Gtk::SelectionData& selection_data, guint info, guint time);
			boost::signals::connection update_treeview_connection;
			void update_treeview();
			DispatcherMarshaller dispatcher; // Execute a function in the gui thread

			/* searches */
			SearchID search_id;
			boost::signals::connection sig_search_tracks_connection;
			void sig_search_tracks_handler(const SearchID id, const std::vector<Track>&);
			sigc::connection search_entry_timeout_connection;
			bool search_entry_timeout_handler();

// 			TrackDataBase&        trackdb;
			ClientID              clientid;
// 			uint32                network_search_id;
// 			std::vector<Track>    network_search_result;
			middle_end&           middleend;

			Gtk::ScrolledWindow   scrolledwindow;
			Gtk::HBox             search_hbox;
			Gtk::Label            search_label;
			Gtk::Entry            search_entry;
			Gtk::VBox             vbox;
			Gtk::VBox             search_vbox;
			gmpmpc_track_treeview treeview;
			Gtk::Button           add_to_wishlist_button;

			boost::mutex          treeview_update_mutex;
	};
#endif // GMPMPC_TRACKDB_H
