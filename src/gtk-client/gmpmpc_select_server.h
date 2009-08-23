#ifndef GMPMPC_SELECT_SERVER_H
	#define GMPMPC_SELECT_SERVER_H
	#include <gtkmm/window.h>
	#include <gtkmm/statusbar.h>
	#include <gtkmm/scrolledwindow.h>
	#include <boost/signal.hpp>
	#include <gtkmm/window.h>
	#include <vector>
	#include <gtkmm/liststore.h>
	#include <gtkmm/treeview.h>
	#include <gtkmm/frame.h>
	#include <gtkmm/button.h>
	#include "../network-handler.h"

	class gmpmpc_select_server_window : public Gtk::Window {
		public:
			gmpmpc_select_server_window();
// 			~gmpmpc_select_server_window();
// 			void update_serverlist(const std::vector<server_info>& si);
			void add_servers(const std::vector<server_info>& si);
			void remove_servers(const std::vector<server_info>& si);

			void connection_accepted(ipv4_socket_addr addr, ClientID id);
			boost::signal<void(ipv4_socket_addr)> sig_connect_to_server;
// 			boost::signal<void(void)>             cancel_signal;
			boost::signal<void(std::string)>      status_message_signal;

		private:
			ipv4_socket_addr target_server;
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

			void save_selected();
			void restore_selected();
			ipv4_socket_addr selected_addr;
			void on_cancel_button_click();
			void on_connect_button_click();
			bool on_serverlist_double_click(GdkEventButton *event);
			bool focus_connect_button(GdkEventWindowState* event); // A tad hacky, but it saves us an extra function doing the same...
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
