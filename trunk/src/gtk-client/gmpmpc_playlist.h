#ifndef GMPMPC_PLAYLIST_H
	#define GMPMPC_PLAYLIST_H
	#include "../playlist_management.h"
	#include "../packet.h"
	#include "../synced_playlist.h"
	#include "../middle_end.h"
	#include "track_treeview.h"
	#include <gtkmm/box.h>
	#include <gtkmm/frame.h>
	#include <gtkmm/button.h>
	#include <gtkmm/treeview.h>
	#include <gtkmm/scrolledwindow.h>
	#include <boost/signal.hpp>

	class gmpmpc_playlist_widget : public Gtk::Frame {
		public:
			gmpmpc_playlist_widget(middle_end& m);
			~gmpmpc_playlist_widget();
			IPlaylistRef sig_update_playlist_handler();
			void add_to_wishlist(Track& track);

			boost::signal<void(messageref)> send_message_signal;
			boost::signal<void(TrackID, int)> vote_signal;

		private:
			middle_end& middleend;
			void on_vote_min_button_clicked();
			IPlaylistRef treeview;
			boost::signals::connection sig_update_playlist_connection;

			Gtk::ScrolledWindow scrolledwindow;
			Gtk::VBox           vbox;
			Gtk::Button         vote_min_button;
			SyncedPlaylist wish_list;
	};

#endif // GMPMPC_PLAYLIST_H
