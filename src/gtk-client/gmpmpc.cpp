#include "gmpmpc.h"
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <signal.h>
#include <gtkmm/stock.h>

using namespace std;
namespace po = boost::program_options;

GtkMpmpClientWindow::GtkMpmpClientWindow(network_handler* nh, TrackDataBase* tdb) {
	networkhandler = nh;
	trackdb = tdb;
	trackdb->add_directory("/home/dafox/sharedfolder/music/");
	trackdb_widget = new gmpmpc_trackdb_widget(trackdb, ClientID(-1));
	construct_gui();
}

GtkMpmpClientWindow::~GtkMpmpClientWindow() {
	delete trackdb_widget;
}

void GtkMpmpClientWindow::construct_gui() {
	set_title("gmpmpc");
	set_default_size(1024, 640);
	set_position(Gtk::WIN_POS_CENTER);

	/* Create actions */
	m_refActionGroup = Gtk::ActionGroup::create();
	/* File menu */
	m_refActionGroup->add(Gtk::Action::create("FileMenu", "File"));
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
				"      <menuitem action='FilePreferences'/>"
				"    <separator/>"
				"      <menuitem action='FileQuit'/>"
				"    </menu>"
				"  </menubar>"
				"</ui>";

	m_refUIManager->add_ui_from_string(ui_info);
	menubar_ptr = (Gtk::Menu*)m_refUIManager->get_widget("/MenuBar");

	playlist_scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	playlist_scrolledwindow.add(playlist_treeview);

	playlist_frame.add(playlist_scrolledwindow);

	main_paned.add(playlist_frame);
	main_paned.add(*trackdb_widget);

	main_vbox.pack_start(*menubar_ptr, Gtk::PACK_SHRINK);
	main_vbox.add(main_paned);
	main_vbox.pack_start(statusbar, Gtk::PACK_SHRINK);

	add(main_vbox);

	playlist_frame.set_label("Playlist:");
	statusbar.push("Ready.", 0);

	show_all();
	main_paned.set_position(main_paned.get_width()/2);
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
