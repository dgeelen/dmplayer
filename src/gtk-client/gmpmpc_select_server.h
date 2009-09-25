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
	#include <gtkmm/notebook.h>
	#include <gtkmm/entry.h>
	#include <gtkmm/label.h>
	#include "../network-handler.h"
	#include "../middle_end.h"
	#include "dispatcher_marshaller.h"
	#include <boost/signals.hpp>

	class gmpmpc_select_server_window : public Gtk::Window {
		public:
			gmpmpc_select_server_window(middle_end& _middleend);
			~gmpmpc_select_server_window();
			boost::signal<void(std::string)>      status_message_signal;

		private:
			middle_end& middleend;
			boost::signals::connection sig_connect_to_server_failure_connection;
			boost::signals::connection sig_connect_to_server_success_connection;
			boost::signals::connection sig_servers_added_connection;
			boost::signals::connection sig_servers_removed_connection;
			boost::signals::connection sig_selection_type_notebook_switch_page_connection;
			DispatcherMarshaller dispatcher; // Execute a function in the gui thread
			ipv4_socket_addr target_server;
			
			class ModelColumns : public Gtk::TreeModelColumnRecord {
				public:
					ModelColumns() {
						add(name);
// 						add(ping);
						add(addr_str);
						add(addr);
					}

					Gtk::TreeModelColumn<Glib::ustring>    name;
// 					Gtk::TreeModelColumn<Glib::ustring>    ping;
					Gtk::TreeModelColumn<Glib::ustring>    addr_str;
					Gtk::TreeModelColumn<ipv4_socket_addr> addr;
			};
			ModelColumns m_Columns;

			void save_selected();
			void restore_selected();
			void connection_accepted(ipv4_socket_addr addr, ClientID id);
			void connection_denied(ipv4_socket_addr addr, std::string reason);
			void add_servers(const std::vector<server_info>& si);
			void remove_servers(const std::vector<server_info>& si);
			ipv4_socket_addr selected_addr;
			void on_cancel_button_click();
			void on_connect_button_click();
			bool on_serverlist_double_click(GdkEventButton *event);
			void on_selection_type_notebook_switch_page(GtkNotebookPage* page, guint page_num);
			bool update_focus(GdkEventWindowState* event);
			Gtk::Entry          manual_ip_entry;
			Gtk::TreeView       serverlist;
			Gtk::Button         connect_button;
			Gtk::Button         cancel_button;
			Gtk::Statusbar      statusbar;
			Gtk::VBox           vbox;
			Gtk::HBox           hbox;
			Gtk::VBox           manual_ip_vbox;
			Gtk::Notebook       selection_type_notebook;
			Gtk::Label          manual_ip_label;
			Gtk::ScrolledWindow scroll;
	};
#endif // GMPMPC_SELECT_SERVER_H
