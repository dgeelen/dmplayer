#ifndef GMPMPC_PLAYLIST_H
	#define GMPMPC_PLAYLIST_H
	#include "../playlist_management.h"
	#include <gtkmm/frame.h>
	#include <gtkmm/scrolledwindow.h>
	#include <gtkmm/treeview.h>

	class gmpmpc_playlist_widget : public Gtk::Frame, public PlaylistVector {
		public:
			gmpmpc_playlist_widget();
		private:
			Gtk::ScrolledWindow scrolledwindow;
			Gtk::TreeView       treeview;
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
