#include "gmpmpc_select_server.h"
#include <boost/bind.hpp>
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <gtkmm/treeselection.h>
#include <glibmm/main.h>

gmpmpc_select_server_window::gmpmpc_select_server_window(middle_end& _middleend)
: middleend(_middleend)
{
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
	signal_window_state_event().connect(sigc::mem_fun(*this, &gmpmpc_select_server_window::focus_connect_button));
	focus_connect_button(NULL);

	serverlist.add_events(Gdk::BUTTON_PRESS_MASK);
	serverlist.signal_button_press_event().connect(sigc::mem_fun(*this, &gmpmpc_select_server_window::on_serverlist_double_click), false);

	Glib::RefPtr<Gtk::ListStore> refListStore = Gtk::ListStore::create(m_Columns);
	serverlist.set_model(refListStore);
	serverlist.append_column("Name", m_Columns.name);
// 	serverlist.append_column("Ping", m_Columns.ping);
	serverlist.append_column("Address", m_Columns.addr_str);

	sig_connect_to_server_success_connection =
		middleend.sig_connect_to_server_success.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::connection_accepted, this, _1, _2)));
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
}

bool gmpmpc_select_server_window::focus_connect_button(GdkEventWindowState* event) {
	connect_button.grab_focus();
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

	if(store->children().size() == 0) {                       // If the server list was empty
		if(added_server_list.size() > 0) {                      // And there now is a server present
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
}

bool gmpmpc_select_server_window::on_serverlist_double_click(GdkEventButton *event) {
	if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		on_connect_button_click();
	}
	Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(*this, &gmpmpc_select_server_window::focus_connect_button), (GdkEventWindowState*)NULL));
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
		target_server = (*sel->get_selected())[m_Columns.addr];
// 		target_server.first.array[3] = 32;
// 		sig_connect_to_server(target_server);
		middleend.connect_to_server(target_server, 0);
	}
}

void gmpmpc_select_server_window::on_cancel_button_click() {

	if(connect_button.sensitive()) {
		hide();
	}
	else {
// 		sig_cancel_connect_to_server(target_server);
		middleend.cancel_connect_to_server(target_server);
		connect_button.set_sensitive(true);
		serverlist.set_sensitive(true);
	}
	statusbar.pop();
}

void gmpmpc_select_server_window::connection_accepted(ipv4_socket_addr addr, ClientID id) {
	connect_button.set_sensitive(true);
	serverlist.set_sensitive(true);
	statusbar.pop();
	Glib::RefPtr<Gtk::TreeSelection> sel = serverlist.get_selection();
	if(sel->count_selected_rows() == 1) {
		status_message_signal(STRFORMAT("Connected to '%s'.", (*sel->get_selected())[m_Columns.name]));
	}
	hide();
}
