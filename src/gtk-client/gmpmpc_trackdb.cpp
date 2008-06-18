#include "gmpmpc.h"
#include "gmpmpc_trackdb.h"
#include "../error-handling.h"
#include "../playlist_management.h"
#include <string>
#include <vector>
#include <glib.h>
using namespace std;
int treeview_trackdb_drag_data_received_signal_id = 0;
int entry_trackdb_insert_text_signal_id = 0;
int entry_trackdb_delete_text_signal_id = 0;

volatile bool update_treeview_trackdb = false;

extern GladeXML* main_window;
TrackDataBase trackDB;

enum {
	TRACKDB_TREE_COLUMN_SONG_ID=0,
	TRACKDB_TREE_COLUMN_SONG_TITLE,
	TRACKDB_TREE_COLUMN_COUNT
};

void entry_trackdb_insert_text(GtkEntry    *entry,
                               const gchar *text,
                               gint         length,
                               gint        *position,
                               gpointer     data) {
	if(!update_treeview_trackdb) {
		update_treeview_trackdb = true;
		g_timeout_add(250, update_treeview, NULL);
	}
}

void entry_trackdb_delete_text(GtkEntry    *entry,
                               const gchar *text,
                               gint         length,
                              gint        *position,
                               gpointer     data) {
	if(!update_treeview_trackdb) {
		update_treeview_trackdb = true;
		g_timeout_add(250, update_treeview, NULL);
	}
}

bool trackdb_uninitialize() {
	disconnect_signal(main_window, treeview_trackdb, drag_data_received);
	return true;
}

bool trackdb_initialize() {
	GtkTreeStore *tree_store_trackdb = gtk_tree_store_new(TRACKDB_TREE_COLUMN_COUNT,
	                                                       G_TYPE_STRING,
	                                                       G_TYPE_STRING
	                                                      );
	/* Connect View to Store */
	try_with_widget(main_window, treeview_trackdb, tv) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(tree_store_trackdb));
		g_object_unref(tree_store_trackdb);
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), true);
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), true);
		DISPLAY_TREEVIEW_COLUMN(tv, "ID", text, TRACKDB_TREE_COLUMN_SONG_ID);
		DISPLAY_TREEVIEW_COLUMN(tv, "Title", text, TRACKDB_TREE_COLUMN_SONG_TITLE);

		GtkTargetEntry tgt_file;
		tgt_file.target   = "text/uri-list";
		tgt_file.flags    = GTK_TARGET_OTHER_APP;
		tgt_file.info     = 0;
		GtkTargetEntry tgt_text;
		tgt_text.target   = "text/plain"; // 		URL == text/plain?
		tgt_text.flags    = GTK_TARGET_OTHER_APP;
		tgt_text.info     = 1;
		GtkTargetEntry tgt_string;
		tgt_string.target = "STRING";
		tgt_string.flags  = GTK_TARGET_OTHER_APP;
		tgt_string.info   = 2;
		GtkTargetEntry targets[] = {tgt_file, tgt_text, tgt_string};
		//NOTE: Everybody uses (GDK_ACTION_MOVE | GDK_ACTION_COPY), but wtf does it mean?
		//      GTK docs fail to explain... :-(
		gtk_drag_dest_set(tv, GTK_DEST_DEFAULT_ALL, targets, 3, (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY));
		try_connect_signal(main_window, treeview_trackdb, drag_data_received);
		dcerr(GTK_WIDGET_NO_WINDOW(tv));
		dcerr(treeview_trackdb_drag_data_received_signal_id);

		try_connect_signal(main_window, entry_trackdb, insert_text);
		try_connect_signal(main_window, entry_trackdb, delete_text);

		return true;
	}
	return false;
}

string urldecode(string s) { //http://www.koders.com/cpp/fid6315325A03C89DEB1E28732308D70D1312AB17DD.aspx
	string buffer = "";
	int len = s.length();

	for (int i = 0; i < len; i++) {
		int j = i ;
		char ch = s.at(j);
		if (ch == '%'){
			char tmpstr[] = "0x0__";
			int chnum;
			tmpstr[3] = s.at(j+1);
			tmpstr[4] = s.at(j+2);
			chnum = strtol(tmpstr, NULL, 16);
			buffer += chnum;
			i += 2;
		} else {
			buffer += ch;
		}
	}
	return buffer;
}

vector<string> urilist_convert(string urilist) {
	vector<string> files;
	int begin = urilist.find("file://", 0);
	int end = urilist.find("\r\n", begin);
	while( end != string::npos) {
		files.push_back(urldecode(urilist.substr(begin + 7, end-begin-7)));
		begin = urilist.find("file://", end);
		if(begin == string::npos) break;
		end = urilist.find("\r\n", begin);
	}
	return files;
}

gboolean update_treeview(void *data) {
	try_with_widget(main_window, treeview_trackdb, tv) {
		GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
		GtkTreeModel* model = GTK_TREE_MODEL(store);
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */

		gtk_tree_store_clear(store);
		MetaDataMap m;
		try_with_widget(main_window, entry_trackdb, txt) {
			m["FILENAME"] = gtk_entry_get_text((GtkEntry*)txt);
		}
		Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), m);
		vector<LocalTrack> s = trackDB.search(query);
		BOOST_FOREACH(LocalTrack& tr, s) {
			Track track(TrackID(ClientID(0),tr.id), tr.metadata );
			GtkTreeIter iter;
			gtk_tree_store_append(store, &iter, NULL);
			char id[10];
			char filename[1024];
			snprintf(id, sizeof(id), "%04x:%04x", int(track.id.first), int(track.id.second));
			MetaDataMap::const_iterator i = track.metadata.find("FILENAME");
			snprintf(filename, sizeof(filename), "%s", i->second.c_str());
			gtk_tree_store_set(store, &iter,
												TRACKDB_TREE_COLUMN_SONG_ID, id,
												TRACKDB_TREE_COLUMN_SONG_TITLE, filename,
												-1);
		}

		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model); /* Re-attach model to view */
	}
	update_treeview_trackdb = false;
	return false;
}

void treeview_trackdb_drag_data_received(GtkWidget *widget,
                                         GdkDragContext *dc,
                                         gint x,
                                         gint y,
                                         GtkSelectionData *selection_data,
                                         guint info,
                                         guint t,
                                         gpointer data
                                        ) {
	dcerr("");
	switch(info) {
		case 0: /* File(s) */
		case 1: /* URL(s) */
		case 2: /* String(s) */
			BOOST_FOREACH(string s, urilist_convert((char*)selection_data->data)) {
				trackDB.add_directory(s);
			}
			update_treeview(NULL);
		default:
		break;
	}
}
