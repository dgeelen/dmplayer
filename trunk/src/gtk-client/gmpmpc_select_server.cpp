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
int button_cancel_server_selection_clicked_signal_id = 0;
int window_select_server_destroy_signal_id           = 0;

enum {
  SERVER_LIST_COLUMN_NAME,
  SERVER_LIST_COLUMN_PING,
  SERVER_LIST_COLUMN_LAST_SEEN,
  SERVER_LIST_COLUMN_SOCK_ADDR,
  SERVER_LIST_COLUMN_COUNT
};

void select_server_update_treeview( const vector<server_info>& si) {
/* TODO:
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

  g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it * /

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view * /

  ... insert a couple of thousand rows ...

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view * /

  g_object_unref(model);
*/

	try_with_widget(select_server_window, treeview_discovered_servers, tv) {
		GtkListStore *list_store_servers = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
		gtk_list_store_clear(list_store_servers);
		GtkTreeIter iter;
		for(vector<server_info>::const_iterator i = si.begin(); i != si.end(); ++i ) {
// 			dcerr(*i << "\n");
			gtk_list_store_append(list_store_servers, &iter);
			gtk_list_store_set (list_store_servers, &iter,
													SERVER_LIST_COLUMN_NAME, i->name.c_str(),
													SERVER_LIST_COLUMN_PING, i->ping_micro_secs,
													SERVER_LIST_COLUMN_LAST_SEEN,  i->ping_last_seen,
													SERVER_LIST_COLUMN_SOCK_ADDR, i->sock_addr.std_str().c_str(),
													-1);
		}
		gtk_list_store_append(list_store_servers, &iter);
// 		gtk_list_store_remove(list_store_servers, &iter);
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
		/* Initialise store for server treeview with Name, Ping, Last Seen, Address */
		GtkListStore *list_store_servers = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING);

		/* Connect View to Store */
		try_with_widget(select_server_window, treeview_discovered_servers, tv) {
			gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(list_store_servers));
			g_object_unref(list_store_servers);
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
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_LIST_COLUMN_NAME); /* What column (of liststore) to render */
			/* Column 2 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Ping");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_LIST_COLUMN_PING); /* What column (of liststore) to render */
			/* Column 3 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Last seen");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_LIST_COLUMN_LAST_SEEN); /* What column (of liststore) to render */
			/* Column 4 */
			col = gtk_tree_view_column_new();
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_set_title(col, "Address");
			gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col); /* pack tree view column into tree view */
			gtk_tree_view_column_pack_start(col, renderer, TRUE); /* pack cell renderer into tree view column */
			gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_LIST_COLUMN_SOCK_ADDR); /* What column (of liststore) to render */
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

