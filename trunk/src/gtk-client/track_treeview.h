#ifndef GMPMPC_TRACK_TREEVIEW_H
	#define GMPMPC_TRACK_TREEVIEW_H
	#include "../playlist_management.h"
	#include <gtkmm/treeview.h>
	#include <gtkmm/liststore.h>
	#include "dispatcher_marshaller.h"

	class gmpmpc_track_treeview : public Gtk::TreeView, public PlaylistVector {
		public:
			gmpmpc_track_treeview();
			virtual void  clear();
			        void _clear();
			virtual void  add(const Track& track);
			        void _add(const Track& track);
			virtual void  batch_add(const std::vector<Track> tracklist);
			        void _batch_add(const std::vector<Track> tracklist);
			virtual void  remove(uint32 pos);
			        void _remove(uint32 pos);
			virtual void  insert(uint32 pos, const Track& track);
			        void _insert(uint32 pos, const Track& track);
			virtual void  move(uint32 from, uint32 to);
			        void _move(uint32 from, uint32 to);

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
			DispatcherMarshaller dispatcher; // Execute a function in the gui thread
	};
#endif //GMPMPC_TRACK_TREEVIEW_H
