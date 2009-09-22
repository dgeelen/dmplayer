#include "gmpmpc_select_server.h"
#include <boost/bind.hpp>
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <gtkmm/treeselection.h>
#include <glibmm/main.h>
#include <gtkmm/messagedialog.h>

gmpmpc_select_server_window::gmpmpc_select_server_window(middle_end& _middleend)
: middleend(_middleend)
{
	set_title( "Select a server" );
	set_default_size( 320, 240);
	set_position( Gtk::WIN_POS_CENTER_ON_PARENT );

	scroll.add(serverlist);
	scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	selection_type_notebook.append_page(scroll, "Server list");
	manual_ip_label.set_text("Please enter the IPv4 address (xxx.xxx.xxx.xxx, with 0 <= xxx <= 255) of the server you wish to connect to.");
	manual_ip_label.set_line_wrap(true);
	manual_ip_label.set_size_request(320,-1);
	manual_ip_vbox.set_border_width(32);
	manual_ip_vbox.add(manual_ip_label);
	manual_ip_entry.set_size_request(320,-1);
	manual_ip_entry.set_max_length(3+1+3+1+3+1+3);
 	manual_ip_entry.set_width_chars(3+1+3+1+3+1+3+1);
	manual_ip_entry.set_activates_default(true);
	manual_ip_vbox.add(manual_ip_entry);
	selection_type_notebook.append_page(manual_ip_vbox, "Manual connect");
	#ifdef DEBUG
	manual_ip_entry.set_text("127.0.0.1");
	selection_type_notebook.set_current_page(1);
	#else
	manual_ip_entry.set_text("131.155.99.208");
	#endif
	vbox.add(selection_type_notebook);
	cancel_button.set_label("Cancel");
	hbox.add(cancel_button);
	connect_button.set_label("Connect");
	hbox.add(connect_button);
	vbox.pack_start(hbox, Gtk::PACK_SHRINK);
	vbox.pack_start(statusbar, Gtk::PACK_SHRINK);
	add(vbox);
	connect_button.set_flags(Gtk::CAN_DEFAULT); // NOTE: set_can_default(true) does not work? (is not a member of ...)
	set_default(connect_button);

	cancel_button.signal_clicked().connect(boost::bind(&gmpmpc_select_server_window::on_cancel_button_click, this));
	connect_button.signal_clicked().connect(boost::bind(&gmpmpc_select_server_window::on_connect_button_click, this));
	selection_type_notebook.signal_switch_page().connect(boost::bind(&gmpmpc_select_server_window::on_selection_type_notebook_switch_page, this, _1, _2));
	signal_window_state_event().connect(sigc::mem_fun(*this, &gmpmpc_select_server_window::update_focus));
	connect_button.grab_focus();

	serverlist.add_events(Gdk::BUTTON_PRESS_MASK);
	serverlist.signal_button_press_event().connect(sigc::mem_fun(*this, &gmpmpc_select_server_window::on_serverlist_double_click), false);

	Glib::RefPtr<Gtk::ListStore> refListStore = Gtk::ListStore::create(m_Columns);
	serverlist.set_model(refListStore);
	//FIXME: Appending columns appears to cause HEAP corruption in release mode?! (on exit)
	serverlist.append_column("Name", m_Columns.name);
// 	serverlist.append_column("Ping", m_Columns.ping);
	serverlist.append_column("Address", m_Columns.addr_str);

	sig_connect_to_server_success_connection =
		middleend.sig_connect_to_server_success.connect (
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::connection_accepted, this, _1, _2)));
	sig_connect_to_server_failure_connection = 
		middleend.sig_connect_to_server_failure.connect (
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::connection_denied, this, _1, _2)));
	sig_servers_added_connection =
		middleend.sig_servers_added.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::add_servers, this, _1)));
	sig_servers_removed_connection =
		middleend.sig_servers_removed.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::remove_servers, this, _1)));
	middleend.refresh_server_list();

	set_modal(true);
}

gmpmpc_select_server_window::~gmpmpc_select_server_window() {
	sig_connect_to_server_success_connection.disconnect();
	sig_servers_added_connection.disconnect();
	sig_servers_removed_connection.disconnect();
	sig_connect_to_server_failure_connection.disconnect();
}

void gmpmpc_select_server_window::on_selection_type_notebook_switch_page(GtkNotebookPage* page, guint page_num) {
	if(page_num == 1) 
		manual_ip_entry.grab_focus();
}


bool gmpmpc_select_server_window::update_focus(GdkEventWindowState* event) {
	if(selection_type_notebook.get_current_page() == 0)
		serverlist.grab_focus();
	else
		manual_ip_entry.grab_focus();
	return false;
}

void gmpmpc_select_server_window::save_selected() {
	Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
	if(sel->count_selected_rows() == 1) {
		selected_addr = (*sel->get_selected())[m_Columns.addr];
	}
}

void gmpmpc_select_server_window::restore_selected() {
	Glib::RefPtr<Gtk::TreeModel> model = serverlist.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);
	Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();

	for(Gtk::TreeModel::Children::iterator row = store->children().begin(); row != store->children().end(); ++row) {
		ipv4_socket_addr addr  = (*row)[m_Columns.addr]; //TODO: Fold this into the if statement below
		if(addr == selected_addr) {
			sel->select(row);
			break;
		}
	}
}

void gmpmpc_select_server_window::add_servers(const std::vector<server_info>& added_server_list) {
	Glib::RefPtr<Gtk::TreeModel> model = serverlist.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	save_selected();

	if(store->children().size() == 0) {                           // If the server list was empty
		if(added_server_list.size() > 0) {                        // And there now is a server present
			selected_addr = added_server_list.begin()->sock_addr; // Select a server by default
		}
	}

	BOOST_FOREACH(server_info si, added_server_list) {
		Gtk::TreeModel::iterator i = store->append();
		(*i)[m_Columns.name]     = si.name;
// 		(*i)[m_Columns.ping]     = STRFORMAT("%0.02f", ((long)((double)si.ping_micro_secs) / ((double)10000))/100.0f);
		(*i)[m_Columns.addr_str] = si.sock_addr.std_str().c_str();
		(*i)[m_Columns.addr]     = si.sock_addr;
	}

	restore_selected();
}

void gmpmpc_select_server_window::remove_servers(const std::vector<server_info>& removed_server_list) {
	Glib::RefPtr<Gtk::TreeModel> model = serverlist.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	save_selected();

	BOOST_FOREACH(server_info si, removed_server_list) {
		for(Gtk::TreeModel::Children::iterator row = store->children().begin(); row != store->children().end(); ++row) {
			ipv4_socket_addr addr  = (*row)[m_Columns.addr]; //TODO: Fold this into the if statement below
			if(addr == si.sock_addr) {
				store->erase(row);
				break;
			}
		}
	}

	restore_selected();
	return;
}

bool gmpmpc_select_server_window::on_serverlist_double_click(GdkEventButton *event) {
	if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		on_connect_button_click();
	}
	Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(*this, &gmpmpc_select_server_window::update_focus), (GdkEventWindowState*)NULL));
	return false;
}

void gmpmpc_select_server_window::on_connect_button_click() {
	target_server = ipv4_socket_addr();
	if(selection_type_notebook.get_current_page() == 0) {
		Glib::RefPtr<Gtk::TreeModel> model = serverlist.get_model();
		Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

		Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
		if(sel->count_selected_rows() == 1) {
			target_server = (*sel->get_selected())[m_Columns.addr];
		}
	}
	else { // page 1
		ipv4_addr addr;
		Glib::ustring ipv4addr_str(manual_ip_entry.get_text());
		ipv4addr_str += '.';
		int i = 0;
		while(!ipv4addr_str.empty() && i < 4) {
			size_t ppos = ipv4addr_str.find_first_of('.');
			Glib::ustring nstr = ipv4addr_str.substr(0, ppos);
			ipv4addr_str.erase(0, ppos+1);
			addr.array[i] = atoi(nstr.c_str()); // FIXME: eeeewww...
			++i;
		}
		target_server = ipv4_socket_addr(addr, TCP_PORT_NUMBER);
	}
	if(target_server != ipv4_socket_addr()) {
		connect_button.set_sensitive(false);
		serverlist.set_sensitive(false);
		manual_ip_entry.set_sensitive(false);
		statusbar.pop();
		statusbar.push("Connecting...");
		status_message_signal("Connecting...");
		middleend.connect_to_server(target_server, 0);
	}
}

void gmpmpc_select_server_window::on_cancel_button_click() {
	if(connect_button.sensitive()) {
		hide();
	}
	else {
		middleend.cancel_connect_to_server(target_server);
		connect_button.set_sensitive(true);
		serverlist.set_sensitive(true);
		manual_ip_entry.set_sensitive(true);
	}
	statusbar.pop();
}

void gmpmpc_select_server_window::connection_denied(ipv4_socket_addr addr, std::string reason) {
	if(addr == target_server) {
		Gtk::MessageDialog dialog("Unable to connect to server.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.set_secondary_text(STRFORMAT("Unable to connect to that server:\n\n\"%s\"\n\nPlease select a different server.", reason));
		dialog.run();
		connect_button.set_sensitive(true);
		serverlist.set_sensitive(true);
		manual_ip_entry.set_sensitive(true);
		statusbar.pop();
	}
}

void gmpmpc_select_server_window::connection_accepted(ipv4_socket_addr addr, ClientID id) {
	if(addr == target_server) {
		connect_button.set_sensitive(true);
		serverlist.set_sensitive(true);
		manual_ip_entry.set_sensitive(true);
		statusbar.pop();
		Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
		if(sel->count_selected_rows() == 1) {
			status_message_signal(STRFORMAT("Connected to '%s'.", (*sel->get_selected())[m_Columns.name]));
		}
		hide();
	}
}
