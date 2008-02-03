#include "gmpmpc.h"
#include "gmpmpc_select_server.h"
#include "gmpmpc_select_server_window.glade.h"
#include "../network-handler.h"
#include "../error-handling.h"
#include <vector>

using namespace std;

GladeXML *select_server_window = NULL;
extern network_handler* gmpmpc_network_handler; //FIXME: Global variables == 3vil
/* Signals */
int window_select_server_delete_event_signal_id      = 0;
int window_select_server_destroy_signal_id           = 0;
int button_cancel_server_selection_clicked_signal_id = 0;
int button_accept_server_selection_clicked_signal_id = 0;

enum {
  SERVER_TREE_COLUMN_NAME,
  SERVER_TREE_COLUMN_PING,
  SERVER_TREE_COLUMN_LAST_SEEN,
  SERVER_TREE_COLUMN_SOCK_ADDR,
  SERVER_TREE_COLUMN_COUNT
};

void select_server_update_treeview( const vector<server_info>& si) {
/* TODO: (possibly)
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

  g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it * /

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view * /

  ... insert a couple of thousand rows ...

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view * /

  g_object_unref(model);
*/
	vector<server_info> my_si = si;
	try_with_widget(select_server_window, treeview_discovered_servers, tv) {
		GtkTreeStore* tree_store_servers = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
		GtkTreeModel* tree_model_servers = GTK_TREE_MODEL(tree_store_servers);
		GtkTreeIter invalid_iter;
		invalid_iter.stamp = 0;
		invalid_iter.user_data = NULL;
		invalid_iter.user_data2 = NULL;
		invalid_iter.user_data3 = NULL;
		GtkTreeIter iter_a = invalid_iter, iter_b = invalid_iter, append_new_servers_here = invalid_iter;
		if(gtk_tree_model_get_iter_first(tree_model_servers, &iter_a)) {
			if(gtk_tree_model_iter_children(tree_model_servers, &iter_b, &iter_a)) {
				do {
					char* server_address;
					gtk_tree_model_get(tree_model_servers, &iter_b, 3, &server_address,-1); //server_address should never be NULL after this
					vector<server_info>::iterator s = my_si.end();
					for(vector<server_info>::iterator i = my_si.begin(); i != my_si.end(); ++i ) {
						if( strncmp( server_address, i->sock_addr.std_str().c_str(), i->sock_addr.std_str().size()) ==0 )
							s = i; //assume servers are uniquely identified by their sock_addr
					}
					if(s!=my_si.end()) { // We already know about this server
						gtk_tree_store_set (tree_store_servers, &iter_b,
										SERVER_TREE_COLUMN_NAME, s->name.c_str(),
										SERVER_TREE_COLUMN_PING, s->ping_micro_secs,
										SERVER_TREE_COLUMN_LAST_SEEN,  s->ping_last_seen,
										SERVER_TREE_COLUMN_SOCK_ADDR, s->sock_addr.std_str().c_str(),
										-1);  // Update ping etc
						my_si.erase(s);
					}
					else { // This server no longer exists
						gtk_tree_store_remove(tree_store_servers, &iter_b);
					}
					append_new_servers_here = iter_b;
				} while(gtk_tree_model_iter_next(tree_model_servers, &iter_b));
			}
		} else { //set up structure
			dcerr("structure");
			GtkTreeIter iter;
			gtk_tree_store_append(tree_store_servers, &iter, NULL);
			gtk_tree_store_set(tree_store_servers, &iter, 0, &"Discovered servers", -1);
			iter_a = iter;
			gtk_tree_store_append(tree_store_servers, &iter, NULL);
			gtk_tree_store_set(tree_store_servers, &iter, 0, "Manually added servers", -1);
			dcerr("structure end");
		}
		for(vector<server_info>::iterator i = my_si.begin(); i != my_si.end(); ++i ) {
			if(append_new_servers_here.stamp == invalid_iter.stamp ) {
				gtk_tree_store_append(tree_store_servers, &append_new_servers_here, &iter_a);
			} else  gtk_tree_store_insert_after(tree_store_servers, &append_new_servers_here, &iter_a, &append_new_servers_here);
			gtk_tree_store_set (tree_store_servers, &append_new_servers_here,
													SERVER_TREE_COLUMN_NAME, i->name.c_str(),
													SERVER_TREE_COLUMN_PING, i->ping_micro_secs,
													SERVER_TREE_COLUMN_LAST_SEEN,  i->ping_last_seen,
													SERVER_TREE_COLUMN_SOCK_ADDR, i->sock_addr.std_str().c_str(),
													-1);
		}


// 		for(vector<server_info>::const_iterator i = si.begin(); i != si.end(); ++i ) {
// // 			dcerr(*i << "\n");
// 			gtk_tree_store_append(tree_store_servers, &iter);
// 			gtk_tree_store_set (tree_store_servers, &iter,
// 													SERVER_TREE_COLUMN_NAME, i->name.c_str(),
// 													SERVER_TREE_COLUMN_PING, i->ping_micro_secs,
// 													SERVER_TREE_COLUMN_LAST_SEEN,  i->ping_last_seen,
// 													SERVER_TREE_COLUMN_SOCK_ADDR, i->sock_addr.std_str().c_str(),
// 													-1);
// 		}
// 		gtk_tree_store_append(tree_store_servers, &iter);
// //
	}
}

bool select_server_initialise_window() {
	/* load the interface from the xml buffer */
	select_server_window = glade_xml_new_from_buffer(gmpmpc_select_server_window_glade_definition, gmpmpc_select_server_window_glade_definition_size, NULL, NULL);

	if(select_server_window != NULL) {
		/* Have the delete event (window close) hide the windows and apply setting*/
 		try_connect_signal(select_server_window, window_select_server, delete_event);
		try_connect_signal(select_server_window, window_select_server, destroy);
		try_connect_signal(select_server_window, button_cancel_server_selection, clicked);
		try_connect_signal(select_server_window, button_accept_server_selection, clicked);
		/* Initialise store for server treeview with Name, Ping, Last Seen, Address */
		GtkTreeStore *tree_store_servers = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING);

		/* Connect View to Store */
		try_with_widget(select_server_window, treeview_discovered_servers, tv) {
			gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(tree_store_servers));
			g_object_unref(tree_store_servers);
			gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), true);
			gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), true);

			/* Set up columns that are displayed */
			GtkTreeViewColumn   *col;
			GtkCellRenderer     *renderer;
			/* Column 1 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Server name");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_NAME); /* What column (of treestore) to render */
			/* Column 2 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Ping");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_PING); /* What column (of treestore) to render */
			/* Column 3 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Last seen");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_LAST_SEEN); /* What column (of treestore) to render */
			/* Column 4 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Address");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TREE_COLUMN_SOCK_ADDR); /* What column (of treestore) to render */
		}
	}
	return select_server_window != NULL;
}

void select_server_uninitialise_window() {
	if(select_server_window) {
		disconnect_signal(select_server_window, window_select_server, delete_event);
		disconnect_signal(select_server_window, button_cancel_server_selection, clicked);
		disconnect_signal(select_server_window, window_select_server, destroy);
		GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
		if(wnd) {
			gtk_widget_destroy(wnd);
		}
	}
}

void window_select_server_destroy(GtkWidget *widget, gpointer user_data) {
	select_server_uninitialise_window();
}

gint window_select_server_delete_event(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
	if( select_server_window == NULL) {
		dcerr("window_select_server_delete_event(): Error: select_server_window is NULL!");
		return false; //???
	}
	GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
	gtk_widget_hide_all(wnd);
	gmpmpc_network_handler->server_list_update_signal.disconnect(select_server_update_treeview);
	return true;
}

void imagemenuitem_select_server_activate(GtkWidget *widget, gpointer user_data) {
	if( select_server_window == NULL) {
		dcerr("imagemenuitem_select_server_activate(): Error: select_server_window is NULL!");
		return;
	}
	GtkWidget* wnd = (GtkWidget*)glade_xml_get_widget(select_server_window, "window_select_server");
	gtk_widget_show_all(wnd);
	gmpmpc_network_handler->server_list_update_signal.connect(select_server_update_treeview);
}

void button_cancel_server_selection_clicked(GtkWidget *widget, gpointer user_data) {
	window_select_server_delete_event(widget, NULL, user_data);
}

void button_accept_server_selection_clicked(GtkWidget *widget, gpointer user_data) {
	dcerr("");
}
