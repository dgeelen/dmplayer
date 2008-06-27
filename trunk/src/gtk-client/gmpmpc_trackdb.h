#ifndef GMPMPC_TRACKDB_H
	#define GMPMPC_TRACKDB_H
	#include "../error-handling.h"
	#include "../network-handler.h"
	#include "../playlist_management.h"
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
			gmpmpc_trackdb_widget(TrackDataBase* tdb, ClientID cid);
			void set_clientid(ClientID id);
			boost::signal<void(Track&)>      enqueue_track_signal;
			boost::signal<void(std::string)> status_message_signal;
		private:
			void on_search_entry_changed();
			void on_add_to_wishlist_button_clicked();
			sigc::connection update_treeview_connection;
			bool update_treeview();
			TrackDataBase*        trackdb;
			ClientID              clientid;


			Gtk::ScrolledWindow   scrolledwindow;
			Gtk::HBox             search_hbox;
			Gtk::Label            search_label;
			Gtk::Entry            search_entry;
			Gtk::VBox             vbox;
			Gtk::VBox             search_vbox;
			gmpmpc_track_treeview treeview;
			Gtk::Button           add_to_wishlist_button;
	};
#endif // GMPMPC_TRACKDB_H


// 	#include <vector>
// 	#include <string>
// 	bool trackdb_initialize();
// 	gboolean update_treeview(void *data);
// 	std::vector<std::string> urilist_convert(std::string urilist);
// 	gboolean treeview_trackdb_update(void *data);
// 	void treeview_trackdb_drag_data_received(GtkWidget *widget,
//                                            GdkDragContext *dc,
//                                            gint x,
//                                            gint y,
//                                            GtkSelectionData *selection_data,
//                                            guint info,
//                                            guint t,
//                                            gpointer data
//                                           );