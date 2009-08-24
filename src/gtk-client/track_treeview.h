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
			virtual void  append(const Track& track);
			        void _append(const Track  track);
			virtual void  append(const std::vector<Track>& tracklist);
			        void _append(const std::vector<Track>  tracklist);
			virtual void  insert(const uint32 pos, const Track& track);
			        void _insert(const uint32 pos, const Track  track);
			virtual void  insert(const uint32 pos, const std::vector<Track>& tracklist);
			        void _insert(const uint32 pos, const std::vector<Track>  tracklist);
			virtual void  move(const uint32 from, const uint32 to);
			        void _move(const uint32 from, const uint32 to);
			virtual void  move(const std::vector<std::pair<uint32, uint32> >& from_to_list);
			        void _move(const std::vector<std::pair<uint32, uint32> >  from_to_list);
			virtual void  remove(const uint32 pos);
			        void _remove(const uint32 pos);
			virtual void  remove(const std::vector<uint32>& poslist);
			        void _remove(const std::vector<uint32>  poslist);

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
