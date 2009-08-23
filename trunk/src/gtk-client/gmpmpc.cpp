#include "gmpmpc.h"
#include "gmpmpc_icon.h"
#include <gdkmm/pixbuf.h>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <signal.h>
#include <gtkmm/stock.h>
#include <gtkmm/messagedialog.h>
#include "../network-handler.h"
#include "../util/StrFormat.h"
#include <glib/gthread.h>
#include <ios>
#include <boost/filesystem/fstream.hpp>

using namespace std;
namespace po = boost::program_options;

#define statusicon_pixbuf_data MagickImage

GtkMpmpClientWindow::GtkMpmpClientWindow() {
	middleend.trackdb.add_directory("/home/dafox/sharedfolder/music/");
	trackdb_widget = new gmpmpc_trackdb_widget(middleend.trackdb, ClientID(-1));
	construct_gui();

	guint8* data = new guint8[gmpmpc_icon_data.width * gmpmpc_icon_data.height * gmpmpc_icon_data.bytes_per_pixel];
	GIMP_IMAGE_RUN_LENGTH_DECODE(data,
	                             gmpmpc_icon_data.rle_pixel_data,
	                             gmpmpc_icon_data.width * gmpmpc_icon_data.height,
	                             gmpmpc_icon_data.bytes_per_pixel);

	statusicon_pixbuf = Gdk::Pixbuf::create_from_data(data,
	                                                  Gdk::COLORSPACE_RGB,
	                                                  true,
	                                                  8,
	                                                  256,
	                                                  256,
	                                                  256*4
	                                                  );
	statusicon = Gtk::StatusIcon::create(statusicon_pixbuf);
	statusicon->set_tooltip("gmpmpc\nA GTK+ Client for MPMP");
	statusicon->set_visible(true);

	statusicon->signal_activate().connect(
		boost::bind(&GtkMpmpClientWindow::on_statusicon_activate, this));
	statusicon->signal_popup_menu().connect(
		sigc::mem_fun(*this, &GtkMpmpClientWindow::on_statusicon_popup_menu));

	this->signal_delete_event().connect(
		sigc::mem_fun(*this, &GtkMpmpClientWindow::on_delete_event));

	set_icon(statusicon_pixbuf);
	select_server_window.set_icon(statusicon_pixbuf);

	connected_signals["enqueue_track_signal"] =
		trackdb_widget->enqueue_track_signal.connect(
			dispatcher.wrap(boost::bind(&GtkMpmpClientWindow::on_enqueue_track_signal, this, _1)));
	connected_signals["playlist_send_message_signal"] =
		playlist_widget.send_message_signal.connect(
			boost::bind(&GtkMpmpClientWindow::send_message, this, _1));
	connected_signals["vote_signal"] =
		playlist_widget.vote_signal.connect(
			boost::bind(&GtkMpmpClientWindow::on_vote_signal, this, _1, _2));
	connected_signals["connect_to_server_success_signal"] =
		middleend.sig_connect_to_server_success.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::connection_accepted, &select_server_window, _1, _2)));
// 	connection_handler.connection_accepted_signal.connect(
// 		dispatcher.wrap(boost::bind(&GtkMpmpClientWindow::on_connection_accepted, this, _1)));
// 	connected_signals["playlist_update_signal"] =
// 		connection_handler.playlist_update_signal.connect(
// 			dispatcher.wrap(boost::bind(&GtkMpmpClientWindow::on_playlist_update, this, _1)));

// Requesting of files will be implemented in middleend
// 	connected_signals["request_file_signal"] =
// 		connection_handler.request_file_signal.connect(
// 			boost::bind(&GtkMpmpClientWindow::on_request_file, this, _1));
// 	connected_signals["request_file_result_signal"] =
// 			connection_handler.request_file_result_signal.connect(
// 				boost::bind(&GtkMpmpClientWindow::on_request_file_result, this, _1));

	connected_signals["sig_connect_to_server"] =
		select_server_window.sig_connect_to_server.connect(
			boost::bind(&middle_end::connect_to_server, &middleend, _1, 0));
	connected_signals["sig_cancel_connect_to_server"] =
		select_server_window.sig_cancel_connect_to_server.connect(
			boost::bind(&middle_end::cancel_connect_to_server, &middleend, _1));
	connected_signals["sig_disconnected"] =
		middleend.sig_disconnected.connect(
			dispatcher.wrap(boost::bind(&GtkMpmpClientWindow::on_disconnect_signal, this, _1)));
	connected_signals["sig_update_playlist_handler"] =
		middleend.sig_update_playlist.connect(
			boost::bind(&gmpmpc_playlist_widget::sig_update_playlist_handler, &playlist_widget));


// 	connected_signals["cancel_signal"] =
// 		select_server_window.cancel_signal.connect(
// 			boost::bind(&GtkMpmpClientWindow::on_select_server_cancel, this));
// 	connected_signals["client_message_receive_signal"] =
// 		networkhandler->client_message_receive_signal.connect(
// 			boost::bind(&gmpmpc_connection_handler::handle_message, &connection_handler, _1));
	connected_signals["select_server_window::add_servers"] =
		middleend.sig_servers_added.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::add_servers, &select_server_window, _1)));
	connected_signals["select_server_window::remove_servers"] =
		middleend.sig_servers_removed.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_select_server_window::remove_servers, &select_server_window, _1)));
	connected_signals["trackdb_status_signal"] =
		trackdb_widget->status_message_signal.connect(
			boost::bind(&GtkMpmpClientWindow::set_status_message, this, _1));
	connected_signals["select_server_status_signal"] =
		select_server_window.status_message_signal.connect(
			boost::bind(&GtkMpmpClientWindow::set_status_message, this, _1));

// //WIP//HAX:!!
// 	middleend->networkhandler->start();

	set_status_message("Selecting server.");
	select_server_window.show_all();
	select_server_window.present();
	middleend.start();
}

void GtkMpmpClientWindow::on_disconnect_signal_dialog_response(int response_id) {
	if(response_id == Gtk::RESPONSE_YES) {
		on_menu_file_connect();
	}
}

void GtkMpmpClientWindow::on_disconnect_signal(const std::string reason) {
	Gtk::MessageDialog dialog("Disconnected from server.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_YES_NO, true);
	dialog.set_secondary_text(STRFORMAT("You were disconnected from the server:\n\n\"%s\"\n\nWould you like to connect to a different server now?", reason));
	dialog.signal_response().connect(boost::bind(&GtkMpmpClientWindow::on_disconnect_signal_dialog_response, this, _1));
	dialog.set_default_response(Gtk::RESPONSE_YES);
	dialog.run();
}

GtkMpmpClientWindow::~GtkMpmpClientWindow() {
	typedef pair<std::string, boost::signals::connection> vtype;
	BOOST_FOREACH(vtype signal, connected_signals) {
		signal.second.disconnect();
	}
	delete trackdb_widget;
}

void GtkMpmpClientWindow::on_statusicon_popup_menu(guint button, guint32 activate_time) {
	Gtk::Menu* m = (Gtk::Menu*)(m_refUIManager->get_widget("/Popup"));
	statusicon->popup_menu_at_position(*m, button, activate_time);
}

// void GtkMpmpClientWindow::_on_request_file(message_request_file_ref m, boost::shared_ptr<bool> done) {
// 	MetaDataMap md;
// 	Track q(m->id, md);
// 	vector<LocalTrack> s = middleend.trackdb.search(q);
// 	if(s.size() == 1) {
// 		if(boost::filesystem::exists(s[0].filename)) {
// 			boost::filesystem::ifstream f( s[0].filename, std::ios_base::in | std::ios_base::binary );
// 			uint32 n = 1024 * 32; // Well over 5sec of 44100KHz 2Channel 16Bit WAV
// 			std::vector<uint8> data;
// 			data.resize(1024*1024);
// 			while(!(*done)) {
// 				f.read((char*)&data[0], n);
// 				uint32 read = f.gcount();
// 				if(read) {
// 					std::vector<uint8> vdata;
// 					std::vector<uint8>::iterator from = data.begin();
// 					std::vector<uint8>::iterator to = from + read;
// 					vdata.insert(vdata.begin(), from, to);
// 					message_request_file_result_ref msg(new message_request_file_result(vdata, m->id));
// // 					networkhandler->send_server_message(msg);
// 					if(n<1024*1024) {
// 						n<<=1;
// 					}
// 					//FIXME: this does not take network latency into account (LAN APP!)
// // 					usleep(n*10); // Ease up on CPU usage, starts with ~0.3 sec sleep and doubles every loop
// 				}
// 				else { //EOF or Error reading file
// 					dcerr("EOF or Error reading file");
// 					*done = true;
// 				}
// 			}
// 			dcerr("Transfer complete");
// 		}
// 		else { // Error opening file
// 			dcerr("Warning: could not open " << s[0].filename << ", Sending as-is...");
// 			std::vector<uint8> vdata;
// 			BOOST_FOREACH(char c, s[0].filename.string()) {
// 				vdata.push_back((uint8)c);
// 			}
// 			message_request_file_result_ref msg(new message_request_file_result(vdata, m->id));
// // 			networkhandler->send_server_message(msg);
// 		}
// 	}
// 	// Send final empty message
// 	std::vector<uint8> vdata;
// 	message_request_file_result_ref msg(new message_request_file_result(vdata, m->id));
// // 	networkhandler->send_server_message(msg);
//
// 	boost::mutex::scoped_lock lock(_on_request_file_threads_mutex);
// 	_on_request_file_threads.erase(_on_request_file_threads.find(m->id));
// }
//
// void GtkMpmpClientWindow::on_request_file_result(message_request_file_result_ref m) {
// 	boost::shared_ptr<boost::thread> t;
//
// 	{
// 		dcerr("Aquiring lock");
// 		boost::mutex::scoped_lock lock(_on_request_file_threads_mutex);
// 		dcerr("checking for id");
// 		if(_on_request_file_threads.find(m->id) != _on_request_file_threads.end()) {
// 			dcerr("Aborting request for file " << (m->id.first) << ":" << (m->id.second));
// 			(*_on_request_file_threads[m->id].second) = true;
// 			t = _on_request_file_threads[m->id].first;
// 		}
// 	}
//
// 	if(t) {
// 		dcerr("joining");
// 		t->join();
// 	}
// 	dcerr("done");
// }
//
// void GtkMpmpClientWindow::on_request_file(message_request_file_ref m) {
// 	// We transfer the file in a separate thread so that we can still send/receive messages.
// 	// FIXME: We can only send a (the same) file one at a time
// 	if(_on_request_file_threads.find(m->id) == _on_request_file_threads.end()) {
// 		boost::mutex::scoped_lock lock(_on_request_file_threads_mutex);
// 		boost::shared_ptr<bool> b = boost::shared_ptr<bool>(new bool(false));
// 		boost::shared_ptr<boost::thread> t = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(
// 			boost::bind(&GtkMpmpClientWindow::_on_request_file, this, m, b))));
//
// 		_on_request_file_threads[m->id] = pair<boost::shared_ptr<boost::thread>, boost::shared_ptr<bool> >(t, b);
// 	}
// }

// void GtkMpmpClientWindow::on_connection_accepted(ClientID clientid) {
// 	select_server_window.connection_accepted();
// 	select_server_window./*hide()*/;
// 	trackdb_widget->set_clientid(clientid);
// 	connected_signals["server_list_update_signal"].block();
// }

void GtkMpmpClientWindow::on_enqueue_track_signal(Track& track) {
	playlist_widget.add_to_wishlist(track);
}

// // void GtkMpmpClientWindow::on_playlist_update(message_playlist_update_ref m) {
// // 	playlist_widget.update(m);
// // }

void GtkMpmpClientWindow::on_vote_signal(TrackID id, int type) {
	if(type < 0) {
		message_vote_ref m = message_vote_ref(new message_vote(id, true));
// 		networkhandler->send_server_message(m);
	}
}

// void GtkMpmpClientWindow::on_select_server_connect( ipv4_socket_addr addr ) {
// 	networkhandler->client_disconnect_from_server();
// 	set_status_message(STRFORMAT("Connecting to %s.", addr.std_str()));
// 	networkhandler->client_connect_to_server(addr);
// }

// void GtkMpmpClientWindow::on_select_server_cancel() {
// 	select_server_window.hide();
// }

void GtkMpmpClientWindow::set_status_message(std::string msg) {
	statusbar.pop();
	statusbar.push(msg);
	clear_statusbar_connection.disconnect();
	sigc::slot<bool> slot = sigc::mem_fun(*this, &GtkMpmpClientWindow::clear_status_messages);
	clear_statusbar_connection = Glib::signal_timeout().connect(slot, 5000);
}

void GtkMpmpClientWindow::send_message(messageref m) {
// 	networkhandler->send_server_message(m);
}

bool GtkMpmpClientWindow::clear_status_messages() {
	set_status_message("Ready.");
	return true;
}

void GtkMpmpClientWindow::construct_gui() {
	set_title("gmpmpc");
	set_default_size(1024, 640);
	set_position(Gtk::WIN_POS_CENTER);

	/* Create actions */
	m_refActionGroup = Gtk::ActionGroup::create();
	/* File menu */
	m_refActionGroup->add(Gtk::Action::create("FileMenu", "_File"));
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
				"      <menuitem action='FileSelectServer'/>"
				"      <menuitem action='FilePreferences'/>"
				"      <separator/>"
				"      <menuitem action='FileQuit'/>"
				"    </menu>"
				"  </menubar>"
				"  <popup name='Popup'>"
				"    <menuitem action='FileSelectServer'/>"
				"    <menuitem action='FilePreferences'/>"
				"    <separator/>"
				"    <menuitem action='FileQuit'/>"
				"  </popup>"
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
	present();
	main_paned.set_position(main_paned.get_width()/2);
}

void GtkMpmpClientWindow::on_menu_file_connect() {
	select_server_window.show_all();
	select_server_window.present();
// 	connected_signals["server_list_update_signal"].unblock();
}

void GtkMpmpClientWindow::on_menu_file_preferences() {
	dcerr("Prefs");
}

bool GtkMpmpClientWindow::on_delete_event(GdkEventAny* event) {
	on_statusicon_activate();
	return true;
}

void GtkMpmpClientWindow::on_menu_file_quit() {
	Gtk::Main::quit();
}

void GtkMpmpClientWindow::on_statusicon_activate() {
	if( is_visible() ) {
		if( property_is_active() ) {
			hide();
		}
		else {
			present();
		}
	}
	else {
		show();
	}
}

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

	if(!Glib::thread_supported()) Glib::thread_init();
	Gtk::Main kit(argc, argv);

// 	TrackDataBase tdb;
// 	network_handler nh(listen_port);
	GtkMpmpClientWindow gmpmpc;

	Gtk::Main::run();
	return 0;
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
