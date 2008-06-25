#include "gmpmpc.h"
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <signal.h>
#include <gtkmm/stock.h>
#include "../network-handler.h"
#include "../util/StrFormat.h"

using namespace std;
namespace po = boost::program_options;

GtkMpmpClientWindow::GtkMpmpClientWindow(network_handler* nh, TrackDataBase* tdb) {
	networkhandler = nh;
	trackdb = tdb;
	trackdb->add_directory("/home/dafox/sharedfolder/music/");
	trackdb_widget = new gmpmpc_trackdb_widget(trackdb, ClientID(-1));
	construct_gui();

	connected_signals["enqueue_track_signal"] =
		trackdb_widget->enqueue_track_signal.connect(
			boost::bind(&gmpmpc_connection_handler::send_message_update_playlist, &connection_handler, _1));
	connected_signals["connection_accepted_signal"] =
		connection_handler.connection_accepted_signal.connect(
			boost::bind(&GtkMpmpClientWindow::on_connection_accepted, this));
	connected_signals["connect_signal"] =
		select_server_window.connect_signal.connect(
			boost::bind(&GtkMpmpClientWindow::on_select_server_connect, this, _1));
	connected_signals["cancel_signal"] =
		select_server_window.cancel_signal.connect(
			boost::bind(&GtkMpmpClientWindow::on_select_server_cancel, this));
	connected_signals["client_message_receive_signal"] =
		networkhandler->client_message_receive_signal.connect(
			boost::bind(&gmpmpc_connection_handler::handle_message, &connection_handler, _1));
	connected_signals["server_list_update_signal"] =
 		networkhandler->server_list_update_signal.connect(
 			boost::bind(&gmpmpc_select_server_window::update_serverlist, &select_server_window, _1));
	connected_signals["trackdb_status_signal"] =
		trackdb_widget->status_message_signal.connect(
			boost::bind(&GtkMpmpClientWindow::set_status_message, this, _1));
	connected_signals["select_server_status_signal"] =
			select_server_window.status_message_signal.connect(
				boost::bind(&GtkMpmpClientWindow::set_status_message, this, _1));
 set_status_message("Selecting server.");
	select_server_window.show_all();
}

GtkMpmpClientWindow::~GtkMpmpClientWindow() {
	typedef pair<std::string, boost::signals::connection> vtype;
	BOOST_FOREACH(vtype signal, connected_signals) {
		signal.second.disconnect();
	}
	delete trackdb_widget;
}

void GtkMpmpClientWindow::on_connection_accepted() {
	select_server_window.connection_accepted();
	select_server_window.hide();
	connected_signals["server_list_update_signal"].block();
}

void GtkMpmpClientWindow::on_select_server_connect( ipv4_socket_addr addr ) {
	networkhandler->client_disconnect_from_server();
	set_status_message(STRFORMAT("Connecting to %s.", addr.std_str()));
	networkhandler->client_connect_to_server(addr);
}

void GtkMpmpClientWindow::on_select_server_cancel() {
	connected_signals["server_list_update_signal"].block();
	select_server_window.hide();
}

void GtkMpmpClientWindow::set_status_message(std::string msg) {
	statusbar.pop();
	statusbar.push(msg);
	clear_statusbar_connection.disconnect();
	sigc::slot<bool> slot = sigc::mem_fun(*this, &GtkMpmpClientWindow::clear_status_messages);
	clear_statusbar_connection = Glib::signal_timeout().connect(slot, 5000);
}

bool GtkMpmpClientWindow::clear_status_messages() {
	set_status_message("Ready.");
}

void GtkMpmpClientWindow::construct_gui() {
	set_title("gmpmpc");
	set_default_size(1024, 640);
	set_position(Gtk::WIN_POS_CENTER);

	/* Create actions */
	m_refActionGroup = Gtk::ActionGroup::create();
	/* File menu */
	m_refActionGroup->add(Gtk::Action::create("FileMenu", "File"));
	m_refActionGroup->add(Gtk::Action::create("FileSelectServer", Gtk::Stock::CONNECT),
	                      sigc::mem_fun(*this, &GtkMpmpClientWindow::on_menu_file_connect));
	m_refActionGroup->add(Gtk::Action::create("FilePreferences", Gtk::Stock::PREFERENCES),
	                      sigc::mem_fun(*this, &GtkMpmpClientWindow::on_menu_file_preferences));
	m_refActionGroup->add(Gtk::Action::create("FileQuit", Gtk::Stock::QUIT),
	                      sigc::mem_fun(*this, &GtkMpmpClientWindow::on_menu_file_quit));

	m_refUIManager = Gtk::UIManager::create();
	m_refUIManager->insert_action_group(m_refActionGroup);

	add_accel_group(m_refUIManager->get_accel_group());

	/* Layout the actions */
	Glib::ustring ui_info =
				"<ui>"
				"  <menubar name='MenuBar'>"
				"    <menu action='FileMenu'>"
				"    <menuitem action='FileSelectServer'/>"
				"      <menuitem action='FilePreferences'/>"
				"      <separator/>"
				"      <menuitem action='FileQuit'/>"
				"    </menu>"
				"  </menubar>"
				"</ui>";

	m_refUIManager->add_ui_from_string(ui_info);
	menubar_ptr = (Gtk::Menu*)m_refUIManager->get_widget("/MenuBar");

	main_paned.add(playlist_widget);
	main_paned.add(*trackdb_widget);

	main_vbox.pack_start(*menubar_ptr, Gtk::PACK_SHRINK);
	main_vbox.add(main_paned);
	main_vbox.pack_start(statusbar, Gtk::PACK_SHRINK);

	add(main_vbox);

	statusbar.push("Ready.", 0);

	show_all();
	main_paned.set_position(main_paned.get_width()/2);
}

void GtkMpmpClientWindow::on_menu_file_connect() {
	select_server_window.show();
	connected_signals["server_list_update_signal"].unblock();
}

void GtkMpmpClientWindow::on_menu_file_preferences() {
	dcerr("Prefs");
}

void GtkMpmpClientWindow::on_menu_file_quit() {
	hide();
}

// TrackDataBase& GtkMpmpClientWindow::get_track_database() {
// 	return *trackdb;
// }
// void GtkMpmpClientWindow::set_track_database(TrackDataBase* tdb) {
// 	trackdb = tdb;
// }
// void GtkMpmpClientWindow::set_network_handler(network_handler* nh) {
// 	networkhandler = nh;
// }

int main_impl(int argc, char **argv ) {
	/* Parse commandline options */
	int listen_port;
	bool showhelp;
	string filename;
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", po::bool_switch(&showhelp)                   , "produce help message")
			("port", po::value(&listen_port)->default_value(12345), "TCP Port")
			("file", po::value(&filename)->default_value("")      , "Filename to test with")
	;
	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (showhelp) {
		cout << desc << "\n";
		return 1;
	}
	/* Done parsing commandline options */

	Gtk::Main kit(argc, argv);

	TrackDataBase tdb;
	network_handler nh(listen_port);
	GtkMpmpClientWindow gmpmpc(&nh, &tdb);

// 	gmpmpc.set_track_database(&tdb);
// 	gmpmpc.set_network_handler(&nh);
// 	gmpmpc.get_track_database().add_directory("/home/dafox/sharedfolder/music/");


	Gtk::Main::run(gmpmpc);
	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}

// int main ( int argc, char *argv[] )
// {
// 	// Hide SIGPIPE
// 	sigset_t sigset;
// 	sigemptyset(&sigset);
// 	sigaddset(&sigset, SIGPIPE);
// 	sigprocmask(SIG_SETMASK, &sigset, NULL);
//
// 	int listen_port;
// 	bool showhelp;
// 	string filename;
// 	po::options_description desc("Allowed options");
// 	desc.add_options()
// 			("help", po::bool_switch(&showhelp)                   , "produce help message")
// 			("port", po::value(&listen_port)->default_value(12345), "TCP Port")
// 			("file", po::value(&filename)->default_value("")      , "Filename to test with")
// 	;
// 	po::variables_map vm;
//  	po::store(po::parse_command_line(argc, argv, desc), vm);
// 	po::notify(vm);
// 	if (showhelp) {
// 		cout << desc << "\n";
// 		return 1;
// 	}
//
// 	return 0;
// }
