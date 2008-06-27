#ifndef GMPMPC_PLAYLIST_H
	#define GMPMPC_PLAYLIST_H
	#include "../playlist_management.h"
	#include "../packet.h"
	#include "../synced_playlist.h"
	#include "track_treeview.h"
	#include <gtkmm/box.h>
	#include <gtkmm/frame.h>
	#include <gtkmm/button.h>
	#include <gtkmm/treeview.h>
	#include <gtkmm/scrolledwindow.h>
	#include <boost/signal.hpp>


	class gmpmpc_playlist_widget : public Gtk::Frame {
		public:
			gmpmpc_playlist_widget();
			void update(message_playlist_update_ref m);
			void add_to_wishlist(Track& track);

			boost::signal<void(messageref)> send_message_signal;
			boost::signal<void(TrackID, int)> vote_signal;

		private:
			void on_vote_min_button_clicked();

			Gtk::ScrolledWindow scrolledwindow;
			Gtk::VBox           vbox;
			Gtk::Button         vote_min_button;
			gmpmpc_track_treeview   treeview;
			SyncedPlaylist wish_list;
	};

#endif // GMPMPC_PLAYLIST_H

/*
	bool playlist_initialize();

		class TreeviewPlaylist : public PlaylistVector {
			public:
				TreeviewPlaylist(GtkTreeView* _tv);

				virtual void add(const Track& track);
				virtual void remove(uint32 pos);
				virtual void insert(uint32 pos, const Track& track);
				virtual void move(uint32 from, uint32 to);
				virtual void clear();

			private:
				GtkTreeView* tv;
		};*/
