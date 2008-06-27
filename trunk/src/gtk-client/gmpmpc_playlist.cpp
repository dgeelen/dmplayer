#include "gmpmpc_playlist.h"
#include <boost/bind.hpp>

gmpmpc_playlist_widget::gmpmpc_playlist_widget() {
	scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrolledwindow.add(treeview);
	vbox.add(scrolledwindow);
	vbox.pack_start(vote_min_button, Gtk::PACK_SHRINK);
	Gtk::Frame::add(vbox);
	set_label("Playlist:");
	vote_min_button.set_border_width(3);
	vote_min_button.set_label("Vote MIN");
	vote_min_button.signal_clicked().connect(boost::bind(&gmpmpc_playlist_widget::on_vote_min_button_clicked, this));
}

void gmpmpc_playlist_widget::update(message_playlist_update_ref m) {
	m->apply(&treeview);
}

void gmpmpc_playlist_widget::add_to_wishlist(Track& track) {
	wish_list.add(track);
	messageref msg;

	while (msg = wish_list.pop_msg()) {
		send_message_signal(msg);
	}
}

void gmpmpc_playlist_widget::on_vote_min_button_clicked() {
	Glib::RefPtr<Gtk::TreeModel> model = treeview.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	Glib::RefPtr<Gtk::TreeSelection> sel = treeview.get_selection();
	std::vector<Gtk::TreeModel::Path> selected_rows = sel->get_selected_rows();
	BOOST_FOREACH(Gtk::TreeModel::Path p, selected_rows) {
		Gtk::TreeModel::iterator i = model->get_iter(p);
		Track t = (*i)[treeview.m_Columns.track];
		vote_signal(t.id, -1);
	}
}







































// #include "gmpmpc.h"
// #include "gmpmpc_playlist.h"
// #include "../error-handling.h"
// #include "../playlist_management.h"
// #include "../synced_playlist.h"
//
// extern GladeXML* main_window;
//
// SyncedPlaylist synced_playlist;
//
// enum {
// 	PLAYLIST_TREE_COLUMN_SONG_ID=0,
// 	PLAYLIST_TREE_COLUMN_SONG_TITLE,
// 	PLAYLIST_TREE_COLUMN_COUNT
// };
//
// bool playlist_uninitialize() {
// 	return true;
// }
//
// bool playlist_initialize() {
// 	GtkTreeStore *tree_store_playlist = gtk_tree_store_new(PLAYLIST_TREE_COLUMN_COUNT,
// 	                                                       G_TYPE_STRING,
// 	                                                       G_TYPE_STRING
// 	                                                      );
// 	/* Connect View to Store */
// 	try_with_widget(main_window, treeview_playlist, tv) {
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(tree_store_playlist));
// 		g_object_unref(tree_store_playlist);
// 		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), true);
// 		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), true);
// 		DISPLAY_TREEVIEW_COLUMN(tv, "ID", text, PLAYLIST_TREE_COLUMN_SONG_ID);
// 		DISPLAY_TREEVIEW_COLUMN(tv, "Title", text, PLAYLIST_TREE_COLUMN_SONG_TITLE);
//
// 		GtkTreeSelection *selection;
// 		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (tv));
// 		gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
//
// 		return true;
// 	}
// 	return false;
// }
//
// TreeviewPlaylist::TreeviewPlaylist(GtkTreeView* _tv) {
// 	this->tv = _tv;
// }
//
// // void TreeviewPlaylist::g_add(uint32 pos) {
// // // 	try_with_widget(main_window, treeview_playlist, tv) {
// // // 		dcerr("track: " << track.id.first << ":" << track.id.second);
// // // 		GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
// // // 		GtkTreeModel* model = GTK_TREE_MODEL(store);
// // // 		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
// // // 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */
// // //
// // // 		GtkTreeIter iter;
// // // 		gtk_tree_store_append(store, &iter, NULL);
// // // 		char *id = new char[8+1+8+1];
// // // 		char *filename = new char[1024];
// // // 		snprintf(id, 8+1+8+1, "%08x:%08x", int(track.id.first), int(track.id.second));
// // // 		MetaDataMap::const_iterator i = track.metadata.find("FILENAME");
// // // 		snprintf(filename, 1024, "%s", i->second.c_str());
// // // 		gtk_tree_store_set(store, &iter,
// // // 		                   PLAYLIST_TREE_COLUMN_SONG_ID, id,
// // // 		                   PLAYLIST_TREE_COLUMN_SONG_TITLE, filename,
// // // 		                   -1);
// // //
// // // 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model); /* Re-attach model to view */
// // // 	}
// // }
//
// void TreeviewPlaylist::add(const Track& track) {
// 	PlaylistVector::add(track);
// // 	g_idle_add(g_add(), size());
// }
//
// void TreeviewPlaylist::remove(uint32 pos) {
// 	PlaylistVector::remove(pos);
// }
//
// void TreeviewPlaylist::insert(uint32 pos, const Track& track) {
// 	PlaylistVector::insert(pos, track);
// }
//
// void TreeviewPlaylist::move(uint32 from, uint32 to) {
// 	PlaylistVector::move(from, to);
// }
//
// void TreeviewPlaylist::clear() {
// 	PlaylistVector::clear();
// 	gdk_threads_enter();
// 	dcerr("track: clearing");
// 	GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
// 	GtkTreeModel* model = GTK_TREE_MODEL(store);
// 	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
// 	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */
//
// 	gtk_tree_store_clear(store);
//
// 	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model); /* Re-attach model to view */
// 	gdk_threads_leave();
// }
