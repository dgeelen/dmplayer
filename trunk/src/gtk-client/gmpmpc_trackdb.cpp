#include "gmpmpc_trackdb.h"
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <vector>
#include <boost/bind.hpp>
#include "../util/StrFormat.h"

using namespace std;

gmpmpc_trackdb_widget::gmpmpc_trackdb_widget(TrackDataBase* tdb, ClientID cid) {
	trackdb = tdb;
	clientid = cid;
	scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrolledwindow.add(treeview);
	search_hbox.pack_start(search_label, Gtk::PACK_SHRINK);
	search_vbox.add(search_hbox);
	search_vbox.add(search_entry);
	vbox.add(scrolledwindow);
	vbox.pack_start(add_to_wishlist_button, Gtk::PACK_SHRINK);
	vbox.pack_start(search_vbox, Gtk::PACK_SHRINK);
	add(vbox);

	set_label("TrackDB:");
	search_label.set_label("Search:");
	add_to_wishlist_button.set_label("Enqueue selected");

	update_treeview();

	search_entry.signal_changed().connect(boost::bind(&gmpmpc_trackdb_widget::on_search_entry_changed, this));
	add_to_wishlist_button.signal_clicked().connect(boost::bind(&gmpmpc_trackdb_widget::on_add_to_wishlist_button_clicked, this));
}

void gmpmpc_trackdb_widget::set_clientid(ClientID id) {
	clientid = id;
	update_treeview();
}

void gmpmpc_trackdb_widget::on_add_to_wishlist_button_clicked() {
	Glib::RefPtr<Gtk::TreeModel> model = treeview.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	status_message_signal("Voting...");
	Glib::RefPtr<Gtk::TreeSelection> sel = treeview.get_selection();
	std::vector<Gtk::TreeModel::Path> selected_rows = sel->get_selected_rows();
	BOOST_FOREACH(Gtk::TreeModel::Path p, selected_rows) {
		Gtk::TreeModel::iterator i = model->get_iter(p);
		Track q;
		q = (*i)[treeview.m_Columns.track];
		std::vector<LocalTrack> r = trackdb->search(q);
		Track t(q.id, r[0].metadata);
		enqueue_track_signal(t);
	}
}

bool gmpmpc_trackdb_widget::update_treeview() {
	// TODO: is this reallly neccessary? Shouldn't Glib::signal_timeout ensure this?
	boost::mutex::scoped_lock lock(treeview_update_mutex);

	treeview.clear();
	MetaDataMap m;
	m["FILENAME"] = search_entry.get_text();
	Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), m);
	vector<LocalTrack> s = trackdb->search(query);
	BOOST_FOREACH(LocalTrack lt, s) {
		Track t(TrackID(clientid, lt.id), lt.metadata);
		treeview.add(t);
	}
	return false;
}

void gmpmpc_trackdb_widget::on_search_entry_changed() {
	update_treeview_connection.disconnect();
	sigc::slot<bool> slot = sigc::mem_fun(*this, &gmpmpc_trackdb_widget::update_treeview);
	update_treeview_connection = Glib::signal_timeout().connect(slot, 250);

// 	sigc::connection conn = Glib::signal_timeout().connect(boost::bind<bool>(&gmpmpc_trackdb_widget::update_treeview, this), 250);
}






















// #include "gmpmpc.h"
// #include "gmpmpc_trackdb.h"
// #include "../error-handling.h"
// #include "../playlist_management.h"
// #include "../network-handler.h"
// #include "gmpmpc_playlist.h"
// #include <string>
// #include <vector>
// #include <glib.h>
// #include "../synced_playlist.h"
//
//
// using namespace std;
// int treeview_trackdb_drag_data_received_signal_id = 0;
// int entry_trackdb_insert_text_signal_id = 0;
// int entry_trackdb_delete_text_signal_id = 0;
// int button_vote_clicked_signal_id = 0;
//
// extern TreeviewPlaylist* treeview_playlist;
// extern network_handler* gmpmpc_network_handler;
// extern ClientID gmpmpc_client_id;
// extern SyncedPlaylist synced_playlist;
//
// volatile bool treeview_trackdb_update_trackdb = false;
//
// extern GladeXML* main_window;
// TrackDataBase trackDB;
//
// enum {
// 	TRACKDB_TREE_COLUMN_SONG_ID=0,
// 	TRACKDB_TREE_COLUMN_SONG_TITLE,
// 	TRACKDB_TREE_COLUMN_TRACKPTR,
// 	TRACKDB_TREE_COLUMN_COUNT
// };
//
// void entry_trackdb_insert_text(GtkEntry    *entry,
//                                const gchar *text,
//                                gint         length,
//                                gint        *position,
//                                gpointer     data) {
// 	if(!treeview_trackdb_update_trackdb) {
// 		treeview_trackdb_update_trackdb = true;
// 		g_timeout_add(250, treeview_trackdb_update, NULL);
// 	}
// }
//
// void entry_trackdb_delete_text(GtkEntry    *entry,
//                                const gchar *text,
//                                gint         length,
//                               gint        *position,
//                                gpointer     data) {
// 	if(!treeview_trackdb_update_trackdb) {
// 		treeview_trackdb_update_trackdb = true;
// 		g_timeout_add(250, treeview_trackdb_update, NULL);
// 	}
// }
//
// void button_vote_clicked(GtkButton *widget, gpointer user_data) {
// 	try_with_widget(main_window, treeview_trackdb, tv) {
// 		GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model((GtkTreeView*)tv));
// 		GtkTreeModel* model = GTK_TREE_MODEL(store);
// 		GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)tv);
// 		GtkTreeIter iter;
// 		GList* selected_rows = gtk_tree_selection_get_selected_rows(selection, &model);
// 		GList* selected_row = g_list_first(selected_rows);
// 		while(selected_row != NULL) {
// 			GtkTreePath* path = (GtkTreePath*)(selected_row->data);
// 			GtkTreeIter iter;
// 			gtk_tree_model_get_iter(model, &iter, path);
// 			Track* t;
// 			gtk_tree_model_get(model, &iter, TRACKDB_TREE_COLUMN_TRACKPTR, &t,-1);
//
// // 			dcerr("Voting " << tid->first << ":" << tid->second);
//
// 			synced_playlist.add(*t); //Fixme: Do a query or use a reference
//
// 			selected_row = g_list_next(selected_row);
// 		}
//
// 		g_list_free(selected_rows);
// 	}
// 	messageref msg;
// 	while(msg = synced_playlist.pop_msg()) {
// 		gmpmpc_network_handler->send_server_message(msg);
// 	}
// }
//
// void clear_treeview() {
// 	try_with_widget(main_window, treeview_trackdb, tv) {
// 		GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
// 		GtkTreeModel* model = GTK_TREE_MODEL(store);
// 		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */
//
// 		GtkTreeIter iter;
// 		if(gtk_tree_model_get_iter_first(model, &iter)) {
// 			do {
// 				Track* t;
// 				gtk_tree_model_get(model, &iter, TRACKDB_TREE_COLUMN_TRACKPTR, &t,-1);
// 				delete t;
//
// 				char *c;
// 				gtk_tree_model_get(model, &iter, TRACKDB_TREE_COLUMN_SONG_ID, &c,-1);
// 				delete c;
// 				gtk_tree_model_get(model, &iter, TRACKDB_TREE_COLUMN_SONG_TITLE, &c,-1);
// 				delete c;
// 			} while(gtk_tree_model_iter_next(model, &iter));
// 		}
//
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model); /* Re-attach model to view */
// 		gtk_tree_store_clear(store);
// 	}
// }
//
// bool trackdb_uninitialize() {
// 	disconnect_signal(main_window, treeview_trackdb, drag_data_received);
// 	clear_treeview();
// 	return true;
// }
//
// bool trackdb_initialize() {
// 	GtkTreeStore *tree_store_trackdb = gtk_tree_store_new(TRACKDB_TREE_COLUMN_COUNT,
// 	                                                       G_TYPE_STRING,
// 	                                                       G_TYPE_STRING,
// 	                                                       G_TYPE_POINTER
// 	                                                      );
// 	/* Connect View to Store */
// 	try_with_widget(main_window, treeview_trackdb, tv) {
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(tree_store_trackdb));
// 		g_object_unref(tree_store_trackdb);
// 		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), true);
// 		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), true);
// 		DISPLAY_TREEVIEW_COLUMN(tv, "ID", text, TRACKDB_TREE_COLUMN_SONG_ID);
// 		DISPLAY_TREEVIEW_COLUMN(tv, "Title", text, TRACKDB_TREE_COLUMN_SONG_TITLE);
//
// 		GtkTreeSelection *selection;
// 		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (tv));
// 		gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
//
// 		GtkTargetEntry tgt_file;
// 		tgt_file.target   = "text/uri-list";
// 		tgt_file.flags    = GTK_TARGET_OTHER_APP;
// 		tgt_file.info     = 0;
// 		GtkTargetEntry tgt_text;
// 		tgt_text.target   = "text/plain"; // 		URL == text/plain?
// 		tgt_text.flags    = GTK_TARGET_OTHER_APP;
// 		tgt_text.info     = 1;
// 		GtkTargetEntry tgt_string;
// 		tgt_string.target = "STRING";
// 		tgt_string.flags  = GTK_TARGET_OTHER_APP;
// 		tgt_string.info   = 2;
// 		GtkTargetEntry targets[] = {tgt_file, tgt_text, tgt_string};
// 		//NOTE: Everybody uses (GDK_ACTION_MOVE | GDK_ACTION_COPY), but wtf does it mean?
// 		//      GTK docs fail to explain... :-(
// 		gtk_drag_dest_set(tv, GTK_DEST_DEFAULT_ALL, targets, 3, (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY));
// 		try_connect_signal(main_window, treeview_trackdb, drag_data_received);
// 		dcerr(GTK_WIDGET_NO_WINDOW(tv));
// 		dcerr(treeview_trackdb_drag_data_received_signal_id);
//
// 		try_connect_signal(main_window, entry_trackdb, insert_text);
// 		try_connect_signal(main_window, entry_trackdb, delete_text);
// 		try_connect_signal(main_window, button_vote, clicked);
//
// 		/* Debug */
// 		BOOST_FOREACH(string s, urilist_convert("file:///home/dafox/sharedfolder/music/\r\n")) {
// 			trackDB.add_directory(s);
// 		}
// 		treeview_trackdb_update(NULL);
//
// 		return true;
// 	}
// 	return false;
// }
//
// string urldecode(string s) { //http://www.koders.com/cpp/fid6315325A03C89DEB1E28732308D70D1312AB17DD.aspx
// 	string buffer = "";
// 	int len = s.length();
//
// 	for (int i = 0; i < len; i++) {
// 		int j = i ;
// 		char ch = s.at(j);
// 		if (ch == '%'){
// 			char tmpstr[] = "0x0__";
// 			int chnum;
// 			tmpstr[3] = s.at(j+1);
// 			tmpstr[4] = s.at(j+2);
// 			chnum = strtol(tmpstr, NULL, 16);
// 			buffer += chnum;
// 			i += 2;
// 		} else {
// 			buffer += ch;
// 		}
// 	}
// 	return buffer;
// }
//
// vector<string> urilist_convert(string urilist) {
// 	vector<string> files;
// 	int begin = urilist.find("file://", 0);
// 	int end = urilist.find("\r\n", begin);
// 	while( end != string::npos) {
// 		files.push_back(urldecode(urilist.substr(begin + 7, end-begin-7)));
// 		begin = urilist.find("file://", end);
// 		if(begin == string::npos) break;
// 		end = urilist.find("\r\n", begin);
// 	}
// 	return files;
// }
//
// gboolean treeview_trackdb_update(void *data) {
// 	try_with_widget(main_window, treeview_trackdb, tv) {
// 		GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
// 		GtkTreeModel* model = GTK_TREE_MODEL(store);
// 		clear_treeview();
// 		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */
//
// 		MetaDataMap m;
// 		try_with_widget(main_window, entry_trackdb, txt) {
// 			m["FILENAME"] = gtk_entry_get_text((GtkEntry*)txt);
// 		}
// 		Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), m);
// 		vector<LocalTrack> s = trackDB.search(query);
// 		BOOST_FOREACH(LocalTrack& tr, s) {
// 			Track track(TrackID( gmpmpc_client_id, tr.id), tr.metadata );
// 			GtkTreeIter iter;
// 			gtk_tree_store_append(store, &iter, NULL);
// 			char *id = new char[8+1+8+1];
// 			char *filename = new char[1024];
// 			snprintf(id, 8+1+8+1, "%08x:%08x", int(track.id.first), int(track.id.second));
// 			MetaDataMap::const_iterator i = track.metadata.find("FILENAME");
// 			snprintf(filename, 1024, "%s", i->second.c_str());
// 			Track* t = new Track(track);
// 			gtk_tree_store_set(store, &iter,
// 												TRACKDB_TREE_COLUMN_SONG_ID, id,
// 												TRACKDB_TREE_COLUMN_SONG_TITLE, filename,
// 												TRACKDB_TREE_COLUMN_TRACKPTR, t,
// 												-1);
// 		}
//
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model); /* Re-attach model to view */
// 	}
// 	treeview_trackdb_update_trackdb = false;
// 	return false;
// }
//
// void treeview_trackdb_drag_data_received(GtkWidget *widget,
//                                          GdkDragContext *dc,
//                                          gint x,
//                                          gint y,
//                                          GtkSelectionData *selection_data,
//                                          guint info,
//                                          guint t,
//                                          gpointer data
//                                         ) {
// 	dcerr("");
// 	switch(info) {
// 		case 0: /* File(s) */
// 		case 1: /* URL(s) */
// 		case 2: /* String(s) */
// 			BOOST_FOREACH(string s, urilist_convert((char*)selection_data->data)) {
// 				trackDB.add_directory(s);
// 			}
// 			treeview_trackdb_update(NULL);
// 		default:
// 		break;
// 	}
// }
