#ifndef GMPMPC_SELECT_SERVER_H
	#define GMPMPC_SELECT_SERVER_H
	bool select_server_initialise_window();
	void select_server_uninitialise_window();
	void window_select_server_destroy(GtkWidget *widget, gpointer user_data);
	gint window_select_server_delete_event(GtkWidget *widget, GdkEvent * event, gpointer user_data);
	void imagemenuitem_select_server_activate(GtkWidget *widget, gpointer user_data);
	void button_cancel_server_selection_clicked(GtkWidget *widget, gpointer user_data);
	void button_accept_server_selection_clicked(GtkWidget *widget, gpointer user_data);
#endif // GMPMPC_SELECT_SERVER_H
