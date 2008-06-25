#ifndef GMPMPC_SELECT_SERVER_H
	#define GMPMPC_SELECT_SERVER_H
	#include <gtkmm/window.h>
	#include <gtkmm/statusbar.h>
	#include <gtkmm/scrolledwindow.h>
	#include <boost/signal.hpp>
	#include <vector>
	#include "../network-handler.h"

	class gmpmpc_select_server_window : public Gtk::Window {
		public:
			gmpmpc_select_server_window();
			~gmpmpc_select_server_window();
			void update_serverlist(const std::vector<server_info>& si);
			void connection_accepted();
			boost::signal<void(ipv4_socket_addr)> connect_signal;
			boost::signal<void(void)>             cancel_signal;

		private:
			class ModelColumns : public Gtk::TreeModelColumnRecord {
				public:
					ModelColumns() {
						add(name);
						add(ping);
						add(addr_str);
						add(addr);
					}

					Gtk::TreeModelColumn<Glib::ustring>    name;
					Gtk::TreeModelColumn<Glib::ustring>    ping;
					Gtk::TreeModelColumn<Glib::ustring>    addr_str;
					Gtk::TreeModelColumn<ipv4_socket_addr> addr;
			};
			ModelColumns m_Columns;

			void on_cancel_button_click();
			void on_connect_button_click();
			Gtk::Frame          frame;
			Gtk::VBox           framebox;
			Gtk::VBox           vbox;
			Gtk::HBox           hbox;
			Gtk::ScrolledWindow scroll;
			Gtk::TreeView       serverlist;
			Gtk::Button         connect_button;
			Gtk::Button         cancel_button;
			Gtk::Statusbar      statusbar;
	};
#endif // GMPMPC_SELECT_SERVER_H























// 	bool select_server_initialise_window();
// 	void select_server_uninitialise_window();
// 	void select_server_accepted();
// 	void window_select_server_destroy(GtkWidget *widget, gpointer user_data);
// 	gint window_select_server_delete_event(GtkWidget *widget, GdkEvent * event, gpointer user_data);
// 	void imagemenuitem_select_server_activate(GtkWidget *widget, gpointer user_data);
// 	void button_cancel_server_selection_clicked(GtkWidget *widget, gpointer user_data);
// 	void button_accept_server_selection_clicked(GtkWidget *widget, gpointer user_data);
