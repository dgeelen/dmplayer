#include "gmpmpc.h"
#include "gmpmpc_playlist.h"
#include "../error-handling.h"
#include "../playlist_management.h"

extern GladeXML* main_window;

enum {
	PLAYLIST_TREE_COLUMN_SONG_ID=0,
	PLAYLIST_TREE_COLUMN_SONG_TITLE,
	PLAYLIST_TREE_COLUMN_COUNT
};

bool playlist_uninitialize() {
	return true;
}

bool playlist_initialize() {
	GtkTreeStore *tree_store_playlist = gtk_tree_store_new(PLAYLIST_TREE_COLUMN_COUNT,
	                                                       G_TYPE_STRING,
	                                                       G_TYPE_STRING
	                                                      );
	/* Connect View to Store */
	try_with_widget(main_window, treeview_playlist, tv) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(tree_store_playlist));
		g_object_unref(tree_store_playlist);
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), true);
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), true);
		DISPLAY_TREEVIEW_COLUMN(tv, "ID", text, PLAYLIST_TREE_COLUMN_SONG_ID);
		DISPLAY_TREEVIEW_COLUMN(tv, "Title", text, PLAYLIST_TREE_COLUMN_SONG_TITLE);

		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (tv));
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

		return true;
	}
	return false;
}

TreeviewPlaylist::TreeviewPlaylist(GtkTreeView* _tv) {
	this->tv = _tv;
}

void TreeviewPlaylist::add(const Track& track) {
	PlaylistVector::add(track);
	try_with_widget(main_window, treeview_playlist, tv) {
		gdk_threads_enter();
		dcerr("track: " << track.id.first << ":" << track.id.second);
		GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
		GtkTreeModel* model = GTK_TREE_MODEL(store);
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */

		GtkTreeIter iter;
		gtk_tree_store_append(store, &iter, NULL);
		char *id = new char[8+1+8+1];
		char *filename = new char[1024];
		snprintf(id, 8+1+8+1, "%08x:%08x", int(track.id.first), int(track.id.second));
		MetaDataMap::const_iterator i = track.metadata.find("FILENAME");
		snprintf(filename, 1024, "%s", i->second.c_str());
		gtk_tree_store_set(store, &iter,
		                   PLAYLIST_TREE_COLUMN_SONG_ID, id,
		                   PLAYLIST_TREE_COLUMN_SONG_TITLE, filename,
		                   -1);

		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model); /* Re-attach model to view */
		gdk_threads_leave();
	}
}

void TreeviewPlaylist::remove(uint32 pos) {
	PlaylistVector::remove(pos);
}

void TreeviewPlaylist::insert(uint32 pos, const Track& track) {
	PlaylistVector::insert(pos, track);
}

void TreeviewPlaylist::move(uint32 from, uint32 to) {
	PlaylistVector::move(from, to);
}

void TreeviewPlaylist::clear() {
	PlaylistVector::clear();
}
