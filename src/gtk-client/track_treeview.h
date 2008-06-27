#ifndef GMPMPC_TRACK_TREEVIEW_H
	#define GMPMPC_TRACK_TREEVIEW_H
	#include "../playlist_management.h"
	#include <gtkmm/treeview.h>
	#include <gtkmm/liststore.h>

	class gmpmpc_track_treeview : public Gtk::TreeView, public PlaylistVector {
		public:
			gmpmpc_track_treeview();
			virtual void add(const Track& track);
			virtual void remove(uint32 pos);
			virtual void insert(uint32 pos, const Track& track);
			virtual void move(uint32 from, uint32 to);
			virtual void clear();

			class ModelColumns : public Gtk::TreeModelColumnRecord {
				public:
					ModelColumns() {
						add(trackid);
						add(filename);
						add(track);
					}

					Gtk::TreeModelColumn<Glib::ustring> trackid;
					Gtk::TreeModelColumn<Glib::ustring> filename;
					Gtk::TreeModelColumn<Track>         track;
			};
			ModelColumns m_Columns;
		private:
			Glib::RefPtr<Gtk::TreeModel> model;
			Glib::RefPtr<Gtk::ListStore> store;
	};
#endif //GMPMPC_TRACK_TREEVIEW_H
