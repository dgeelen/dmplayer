#include "track_treeview.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <boost/bind.hpp>

gmpmpc_track_treeview::gmpmpc_track_treeview() {
	Glib::RefPtr<Gtk::ListStore> refListStore = Gtk::ListStore::create(m_Columns);
	set_model(refListStore);
	//FIXME: Appending columns appears to cause HEAP corruption in release mode?! (on exit)
	append_column("TrackID", m_Columns.trackid);
	append_column("Filename", m_Columns.filename);
	get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
	set_rubber_banding(true);

	model = get_model();
	store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);
}

void gmpmpc_track_treeview::clear() {
	dispatcher.wrap(boost::bind(&gmpmpc_track_treeview::_clear, this))();
}

void gmpmpc_track_treeview::add(const Track& t) {
	dispatcher.wrap(boost::bind(&gmpmpc_track_treeview::_add, this, t))();
}

void gmpmpc_track_treeview::batch_add(const std::vector<Track> tracklist) {
	dispatcher.wrap(boost::bind(&gmpmpc_track_treeview::_batch_add, this, tracklist))();
}

void gmpmpc_track_treeview::remove(uint32 pos) {
	dispatcher.wrap(boost::bind(&gmpmpc_track_treeview::_remove, this, pos))();
}

void gmpmpc_track_treeview::insert(uint32 pos, const Track& track) {
	dispatcher.wrap(boost::bind(&gmpmpc_track_treeview::_insert, this, pos, track))();
}

void gmpmpc_track_treeview::move(uint32 from, uint32 to) {
	dispatcher.wrap(boost::bind(&gmpmpc_track_treeview::move, this, from, to))();
}

void gmpmpc_track_treeview::_clear() {
	PlaylistVector::clear();
	store->clear();
}

void gmpmpc_track_treeview::_add(const Track& t) {
	PlaylistVector::add(t);
	Gtk::TreeModel::iterator i = store->append();
	(*i)[m_Columns.trackid]    = STRFORMAT("%08x:%08x", t.id.first, t.id.second);
	(*i)[m_Columns.filename]   = (*t.metadata.find("FILENAME")).second;
	(*i)[m_Columns.track]      = t;
}

void gmpmpc_track_treeview::_batch_add(const std::vector<Track> tracklist) {
	// This is a trick to improve performance:
	// We disconnect the treeview's data-store while we're
	// updating the data in the store so that the tree will not
	// try to keep up to date. This helps when adding a large number of
	// rows to the store.
	// FIXME: Test actual benefit over _add().
	Glib::RefPtr<Gtk::TreeModel> _model = get_model();
	Glib::RefPtr<Gtk::ListStore> _store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(_model);
	set_model(Glib::RefPtr<Gtk::TreeModel>());
	BOOST_FOREACH(Track t, tracklist) {
		PlaylistVector::add(t);
		Gtk::TreeModel::iterator i = _store->append();
		(*i)[m_Columns.trackid]    = STRFORMAT("%08x:%08x", t.id.first, t.id.second);
		(*i)[m_Columns.filename]   = (*t.metadata.find("FILENAME")).second;
		(*i)[m_Columns.track]      = t;
	}
	set_model(_model); /* Re-attach model to view */
}

void gmpmpc_track_treeview::_remove(uint32 pos) {
	//FIXME: Update Store
	PlaylistVector::remove(pos);
}

void gmpmpc_track_treeview::_insert(uint32 pos, const Track& track) {
	//FIXME: Update Store
	PlaylistVector::insert(pos, track);
}

void gmpmpc_track_treeview::_move(uint32 from, uint32 to) {
	//FIXME: Update Store
	PlaylistVector::move(from, to);
}
