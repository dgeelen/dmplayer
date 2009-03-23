#include "track_treeview.h"
#include "../util/StrFormat.h"

gmpmpc_track_treeview::gmpmpc_track_treeview() {
	Glib::RefPtr<Gtk::ListStore> refListStore = Gtk::ListStore::create(m_Columns);
	set_model(refListStore);
	append_column("TrackID", m_Columns.trackid);
	append_column("Filename", m_Columns.filename);
	get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
	set_rubber_banding(true);

	model = get_model();
	store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);
}

void gmpmpc_track_treeview::clear() {
	PlaylistVector::clear();
	store->clear();
}

void gmpmpc_track_treeview::add(const Track& t) {
	PlaylistVector::add(t);
	Gtk::TreeModel::iterator i = store->append();
	(*i)[m_Columns.trackid]    = STRFORMAT("%08x:%08x", t.id.first, t.id.second);
	(*i)[m_Columns.filename]   = (*t.metadata.find("FILENAME")).second;
	(*i)[m_Columns.track]      = t;
}

void gmpmpc_track_treeview::remove(uint32 pos) {
	PlaylistVector::remove(pos);
}

void gmpmpc_track_treeview::insert(uint32 pos, const Track& track) {
	PlaylistVector::insert(pos, track);
}

void gmpmpc_track_treeview::move(uint32 from, uint32 to) {
	PlaylistVector::move(from, to);
}
