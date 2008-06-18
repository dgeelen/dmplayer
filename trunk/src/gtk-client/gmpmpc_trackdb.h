#ifndef GMPMPC_TRACKDB_H
	#define GMPMPC_TRACKDB_H
	bool trackdb_initialize();
	gboolean update_treeview(void *data);
	void treeview_trackdb_drag_data_received(GtkWidget *widget,
                                           GdkDragContext *dc,
                                           gint x,
                                           gint y,
                                           GtkSelectionData *selection_data,
                                           guint info,
                                           guint t,
                                           gpointer data
                                          );
#endif // GMPMPC_TRACKDB_H
