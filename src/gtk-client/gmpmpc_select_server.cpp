#include "gmpmpc_select_server.h"
#include <boost/bind.hpp>
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <gtkmm/treeselection.h>
#include <glibmm/main.h>

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
	signal_window_state_event().connect(sigc::mem_fun(*this, &gmpmpc_select_server_window::focus_connect_button));
	focus_connect_button(NULL);

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

bool gmpmpc_select_server_window::focus_connect_button(GdkEventWindowState* event) {
	connect_button.grab_focus();
	return false;
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
	if(selected.size()>0) {
		sel->select(selected[0]);
	}
	else {
		if((store->children().size() > 0)) {
			sel->select(store->children().begin());
		}
	}
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
