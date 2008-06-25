#include "gmpmpc_select_server.h"
#include <boost/bind.hpp>
#include "../util/StrFormat.h"
#include <gtkmm/treeselection.h>

gmpmpc_select_server_window::gmpmpc_select_server_window() {
	set_title( "Select a server" );
	set_default_size( 512, 320);
	set_position( Gtk::WIN_POS_CENTER_ON_PARENT );
	framebox.add(serverlist);
	hbox.add(cancel_button);
	hbox.add(connect_button);
	framebox.pack_start(hbox, Gtk::PACK_SHRINK);
	frame.add(framebox);
	vbox.add(frame);
	vbox.pack_start(statusbar, Gtk::PACK_SHRINK);
	add(vbox);

	connect_button.set_label("Connect");
	cancel_button.set_label("Cancel");
	frame.set_label("Servers:");

	cancel_button.signal_clicked().connect(boost::bind(&gmpmpc_select_server_window::on_cancel_button_click, this));
	connect_button.signal_clicked().connect(boost::bind(&gmpmpc_select_server_window::on_connect_button_click, this));
	serverlist.add_events(Gdk::BUTTON_PRESS_MASK);
	serverlist.signal_button_press_event().connect(sigc::mem_fun(*this, &gmpmpc_select_server_window::on_serverlist_double_click), false);

	Glib::RefPtr<Gtk::ListStore> refListStore = Gtk::ListStore::create(m_Columns);
	serverlist.set_model(refListStore);
	serverlist.append_column("Name", m_Columns.name);
	serverlist.append_column("Ping", m_Columns.ping);
	serverlist.append_column("Address", m_Columns.addr_str);

	set_modal(true);
}

gmpmpc_select_server_window::~gmpmpc_select_server_window() {
	cancel_signal();
}

void gmpmpc_select_server_window::update_serverlist(const std::vector<server_info>& si) {
	Glib::RefPtr<Gtk::TreeModel> model = serverlist.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
	std::vector<Gtk::TreeModel::Path> selected = sel->get_selected_rows();
	store->clear();
	BOOST_FOREACH(const server_info& s, si) {
		Gtk::TreeModel::iterator i = store->append();
		(*i)[m_Columns.name]     = s.name;
		(*i)[m_Columns.ping]     = STRFORMAT("%0.02f", ((long)((double)s.ping_micro_secs) / ((double)10000))/100.0f);
		(*i)[m_Columns.addr_str] = s.sock_addr.std_str().c_str();
		(*i)[m_Columns.addr]     = s.sock_addr;
	}
	if(selected.size()>0)
		sel->select(selected[0]);
}

bool gmpmpc_select_server_window::on_serverlist_double_click(GdkEventButton *event) {
	if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		on_connect_button_click();
	}
	return false;
}

void gmpmpc_select_server_window::on_connect_button_click() {
	Glib::RefPtr<Gtk::TreeModel> model = serverlist.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
	if(sel->count_selected_rows() == 1) {
		connect_button.set_sensitive(false);
		serverlist.set_sensitive(false);

		statusbar.pop();
		statusbar.push("Connecting...");
		status_message_signal("Connecting...");
		connect_signal((*sel->get_selected())[m_Columns.addr]);
	}
}

void gmpmpc_select_server_window::on_cancel_button_click() {
	if(connect_button.sensitive()) {
		cancel_signal();
	}
	else {
		connect_button.set_sensitive(true);
		serverlist.set_sensitive(true);
	}
	statusbar.pop();
}

void gmpmpc_select_server_window::connection_accepted() {
	connect_button.set_sensitive(true);
	serverlist.set_sensitive(true);
	statusbar.pop();
	Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
	if(sel->count_selected_rows() == 1) {
		status_message_signal(STRFORMAT("Connected to '%s'.", (*sel->get_selected())[m_Columns.name]));
	}
}




































// #include "gmpmpc_connection_handling.h"
// #include "../network-handler.h"
// #include "../util/StrFormat.h"
// #include <vector>
//
// using namespace std;
//
// GladeXML *select_server_window = NULL;
//
// /* Signals */
// int window_select_server_delete_event_signal_id      = 0;
// int window_select_server_destroy_signal_id           = 0;
// int button_cancel_server_selection_clicked_signal_id = 0;
// int button_accept_server_selection_clicked_signal_id = 0;
//
// enum {
//   SERVER_TREE_COLUMN_NAME,
//   SERVER_TREE_COLUMN_PING,
//   SERVER_TREE_COLUMN_LAST_SEEN,
//   SERVER_TREE_COLUMN_SOCK_ADDR,
//   SERVER_TREE_COLUMN_SOCK_ADDR_PTR,
//   SERVER_TREE_COLUMN_COUNT
// };
//
// void select_server_update_treeview( const vector<server_info>& si) {
// /* TODO: (possibly)
//   model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
//
//   g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it * /
//
//   gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view * /
//
//   ... insert a couple of thousand rows ...
//
//   gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view * /
//
//   g_object_unref(model);
// */
// 	vector<server_info> my_si = si;
// 	try_with_widget(select_server_window, treeview_discovered_servers, tv) {
// 		GtkTreeStore* tree_store_servers = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
// 		GtkTreeModel* tree_model_servers = GTK_TREE_MODEL(tree_store_servers);
// 		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
// 		GtkTreeIter invalid_iter;
// 		invalid_iter.stamp = 0;
// 		invalid_iter.user_data = NULL;
// 		invalid_iter.user_data2 = NULL;
// 		invalid_iter.user_data3 = NULL;
// 		GtkTreeIter selected_server_iter = invalid_iter;
// 		ipv4_socket_addr* selected_server_address;
// 		if(gtk_tree_selection_get_selected(selection, &tree_model_servers, &selected_server_iter)) {
// 			gtk_tree_model_get(tree_model_servers, &selected_server_iter, SERVER_TREE_COLUMN_SOCK_ADDR_PTR, &selected_server_address,-1);
// 		}
// 		g_object_ref(tree_model_servers); /* Make sure the model stays with us after the tree view unrefs it */
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), NULL); /* Detach model from view */
// 		GtkTreeIter iter_a = invalid_iter, iter_b = invalid_iter, append_new_servers_here = invalid_iter;
// 		if(gtk_tree_model_get_iter_first(tree_model_servers, &iter_a)) {
// 			if(gtk_tree_model_iter_children(tree_model_servers, &iter_b, &iter_a)) {
// 				do {
// 					char* server_address;
// 					gtk_tree_model_get(tree_model_servers, &iter_b, SERVER_TREE_COLUMN_SOCK_ADDR, &server_address,-1); //server_address should never be NULL after this
// 					vector<server_info>::iterator s = my_si.end();
// 					for(vector<server_info>::iterator i = my_si.begin(); i != my_si.end(); ++i ) {
// 						if( strncmp( server_address, i->sock_addr.std_str().c_str(), i->sock_addr.std_str().size()) == 0 )
// 							s = i; //assume servers are uniquely identified by their sock_addr
// 					}
// 					if(s!=my_si.end()) { // We already know about this server
// 						ipv4_socket_addr* addr = new ipv4_socket_addr(s->sock_addr);
// 						gtk_tree_store_set (tree_store_servers, &iter_b,
// 										SERVER_TREE_COLUMN_NAME, s->name.c_str(),
// 										SERVER_TREE_COLUMN_PING, s->ping_micro_secs,
// 										SERVER_TREE_COLUMN_LAST_SEEN,  s->ping_last_seen,
// 										SERVER_TREE_COLUMN_SOCK_ADDR, s->sock_addr.std_str().c_str(),
// 										SERVER_TREE_COLUMN_SOCK_ADDR_PTR, addr,
// 										-1);  // Update ping etc
// 						my_si.erase(s);
// 					}
// 					else { // This server no longer exists
// 						ipv4_socket_addr* addr;
// 						gtk_tree_model_get(tree_model_servers, &iter_b, SERVER_TREE_COLUMN_SOCK_ADDR, &addr,-1);
// 						if( string(server_address) == STRFORMAT("%s", *selected_server_address) ) {
// 							selected_server_iter = invalid_iter;
// 						}
// 						delete addr;
// 						if(!gtk_tree_store_remove(tree_store_servers, &iter_b)) {
// 							// If iter is no longer valid we don't want to call gtk_tree_model_iter_next
// 							// It is only invalid if the server we deleted was the last in the list
// 							break;
// 						}
// 					}
// 					append_new_servers_here = iter_b;
// 				} while(gtk_tree_model_iter_next(tree_model_servers, &iter_b));
// 			}
// 		} else { //set up structure
// 			GtkTreeIter iter;
// 			gtk_tree_store_append(tree_store_servers, &iter, NULL);
// 			gtk_tree_store_set(tree_store_servers, &iter, 0, "Discovered servers", -1);
// 			iter_a = iter;
// 			gtk_tree_store_append(tree_store_servers, &iter, NULL);
// 			gtk_tree_store_set(tree_store_servers, &iter, 0, "Manually added servers", -1);
// 		}
// 		for(vector<server_info>::iterator i = my_si.begin(); i != my_si.end(); ++i ) {
// 			if(append_new_servers_here.stamp == invalid_iter.stamp ) {
// 				gtk_tree_store_append(tree_store_servers, &append_new_servers_here, &iter_a);
// 			} else  gtk_tree_store_insert_after(tree_store_servers, &append_new_servers_here, &iter_a, &append_new_servers_here);
// 			ipv4_socket_addr* addr = new ipv4_socket_addr(i->sock_addr);
// 			gtk_tree_store_set (tree_store_servers, &append_new_servers_here,
// 													SERVER_TREE_COLUMN_NAME, i->name.c_str(),
// 													SERVER_TREE_COLUMN_PING, i->ping_micro_secs,
// 													SERVER_TREE_COLUMN_LAST_SEEN,  i->ping_last_seen,
// 													SERVER_TREE_COLUMN_SOCK_ADDR, i->sock_addr.std_str().c_str(),
// 													SERVER_TREE_COLUMN_SOCK_ADDR_PTR, addr,
// 													-1);
// 		}
// 		gtk_tree_view_set_model(GTK_TREE_VIEW(tv), tree_model_servers); /* Re-attach model to view */
// 		gtk_tree_view_expand_all(GTK_TREE_VIEW(tv));
// 		if(selected_server_iter.stamp != 0) {
// 			gtk_tree_selection_select_iter(selection, &selected_server_iter);
// 		}
// 	}
// }
//
// bool select_server_initialise_window() {
// 	/* load the interface from the xml buffer */
// 	select_server_window = glade_xml_new_from_buffer(gmpmpc_select_server_window_glade_definition, gmpmpc_select_server_window_glade_definition_size, NULL, NULL);
//
// 	if(select_server_window != NULL) {
// 		/* Have the delete event (window close) hide the windows and apply setting*/
//  		try_connect_signal(select_server_window, window_select_server, delete_event);
// 		try_connect_signal(select_server_window, window_select_server, destroy);
// 		try_connect_signal(select_server_window, button_cancel_server_selection, clicked);
// 		try_connect_signal(select_server_window, button_accept_server_selection, clicked);
// 		/* Initialise store for server treeview with Name, Ping, Last Seen, Address */
// 		GtkTreeStore *tree_store_servers = gtk_tree_store_new(SERVER_TREE_COLUMN_COUNT, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_POINTER);
//
// 		/* Connect View to Store */
// 		try_with_widget(select_server_window, treeview_discovered_servers, tv) {
// 			gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(tree_store_servers));
// 			g_object_unref(tree_store_servers);
// 			gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), true);
// 			gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), true);
//
// 			/* Set up columns that are displayed */
// 			GtkTreeViewColumn   *col;
// 			GtkCellRenderer     *renderer;
// 			/* Column 1 */
// 			col = gtk_tree_view_column_new();
// 			renderer = gtk_cell_renderer_text_new();
// 			gtk_tree_view_column_set_title(col, "Server name");
// 			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
// 			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
// 			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_NAME); /* What column (of treestore) to render */
// 			/* Column 2 */
// 			col = gtk_tree_view_column_new();
// 			renderer = gtk_cell_renderer_text_new();
// 			gtk_tree_view_column_set_title(col, "Ping");
// 			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
// 			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
// 			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_PING); /* What column (of treestore) to render */
// // 			/* Column 3 */
// // 			col = gtk_tree_view_column_new();
// // 			renderer = gtk_cell_renderer_text_new();
// // 			gtk_tree_view_column_set_title(col, "Last seen");
// // 			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
// // 			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
// // 			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_LAST_SEEN); /* What column (of treestore) to render */
// 			/* Column 4 */
// 			col = gtk_tree_view_column_new();
// 			renderer = gtk_cell_renderer_text_new();
// 			gtk_tree_view_column_set_title(col, "Address");
// 			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
// 			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
// 			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_SOCK_ADDR); /* What column (of treestore) to render */
// 		}
// 	}
// 	return select_server_window != NULL;
// }
//
// void select_server_uninitialise_window() {
// 	if(select_server_window) {
// 		disconnect_signal(select_server_window, window_select_server, delete_event);
// 		disconnect_signal(select_server_window, button_cancel_server_selection, clicked);
// 		disconnect_signal(select_server_window, window_select_server, destroy);
// 		GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
// 		if(wnd) {
// 			gtk_widget_destroy(wnd);
// 		}
// 	}
// }
//
// void window_select_server_destroy(GtkWidget *widget, gpointer user_data) {
// 	select_server_uninitialise_window();
// }
//
// gint window_select_server_delete_event(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
// 	if( select_server_window == NULL) {
// 		dcerr("window_select_server_delete_event(): Error: select_server_window is NULL!");
// 		return false; //???
// 	}
// 	GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
// 	gtk_widget_hide_all(wnd);
// 	gmpmpc_network_handler->server_list_update_signal.disconnect(select_server_update_treeview);
// 	return true;
// }
//
// void imagemenuitem_select_server_activate(GtkWidget *widget, gpointer user_data) {
// 	if( select_server_window == NULL) {
// 		dcerr("imagemenuitem_select_server_activate(): Error: select_server_window is NULL!");
// 		return;
// 	}
// 	GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
// 	gtk_widget_show_all(wnd);
// 	gmpmpc_network_handler->server_list_update_signal.connect(select_server_update_treeview);
// }
//
// void button_cancel_server_selection_clicked(GtkWidget *widget, gpointer user_data) {
// 	try_with_widget(select_server_window, button_accept_server_selection, b) {
// 		if(GTK_WIDGET_IS_SENSITIVE(b)) {
// 			window_select_server_delete_event(widget, NULL, user_data);
// 		}
// 		gtk_widget_set_sensitive(b, true);
// 	}
// 	try_with_widget(select_server_window, button_cancel_server_selection, b) {
// 		gtk_button_set_label((GtkButton*)b, "Cancel");
// 	}
// }
//
// void select_server_accepted() {
// 	try_with_widget(select_server_window, button_accept_server_selection, b) {
// 		gtk_widget_set_sensitive(b, true);
// 	}
// 	try_with_widget(select_server_window, button_cancel_server_selection, b) {
// 		gtk_button_set_label((GtkButton*)b, "Cancel");
// 	}
// 	window_select_server_delete_event(NULL, NULL, NULL);
// }
//
// void button_accept_server_selection_clicked(GtkWidget *widget, gpointer user_data) {
// 	try_with_widget(select_server_window, treeview_discovered_servers, tv) {
// 		GtkTreeView*  tree_view_servers  = GTK_TREE_VIEW(tv);
// 		GtkTreeStore* tree_store_servers = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view_servers));
// 		GtkTreeModel* tree_model_servers = GTK_TREE_MODEL(tree_store_servers);
// 		GtkTreeSelection* selection = gtk_tree_view_get_selection(tree_view_servers);
// 		GtkTreeIter iter;
// 		if(gtk_tree_selection_get_selected(selection, &tree_model_servers, &iter)) {
// 			ipv4_socket_addr* server_address;
// 			try_with_widget(select_server_window, button_accept_server_selection, b) {
// 				gtk_widget_set_sensitive(b, false);
// 			}
// 			try_with_widget(select_server_window, button_cancel_server_selection, b) {
// 				gtk_button_set_label((GtkButton*)b, "Abort");
// 			}
// 			gtk_tree_model_get(tree_model_servers, &iter, SERVER_TREE_COLUMN_SOCK_ADDR_PTR, &server_address,-1);
// 			if(server_address) { //FIXME: Bit hacky, better use gtk_tree_selection_set_select_function()
// 				gmpmpc_network_handler->client_message_receive_signal.connect(handle_received_message);
// 				gmpmpc_network_handler->client_connect_to_server( *server_address );
// 			}
// 		}
// 	}
// }
