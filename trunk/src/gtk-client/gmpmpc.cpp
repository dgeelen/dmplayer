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

GtkMpmpClientWindow::GtkMpmpClientWindow(middle_end& _middleend)
	: middleend(_middleend),
	  playlist_widget(middleend),
	  trackdb_widget(middleend),
	  select_server_window(middleend)
{
	construct_gui();

	statusicon->signal_activate().connect(
		boost::bind(&GtkMpmpClientWindow::on_statusicon_activate, this));

	statusicon->signal_popup_menu().connect(
		sigc::mem_fun(*this, &GtkMpmpClientWindow::on_statusicon_popup_menu));

	this->signal_delete_event().connect(
		sigc::mem_fun(*this, &GtkMpmpClientWindow::on_delete_event));

	connected_signals["sig_disconnected"] =
		middleend.sig_disconnected.connect(
			dispatcher.wrap(boost::bind(&GtkMpmpClientWindow::on_disconnect_signal, this, _1)));

	connected_signals["select_server_status_signal"] =
		select_server_window.status_message_signal.connect(
			boost::bind(&GtkMpmpClientWindow::set_status_message, this, _1));

	set_status_message("Selecting server.");
	select_server_window.show_all();
	select_server_window.present();
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
}

void GtkMpmpClientWindow::on_statusicon_popup_menu(guint button, guint32 activate_time) {
	Gtk::Menu* m = (Gtk::Menu*)(m_refUIManager->get_widget("/Popup"));
	statusicon->popup_menu_at_position(*m, button, activate_time);
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
	return true;
}

void GtkMpmpClientWindow::construct_gui() {
	set_title("gmpmpc");
	set_default_size(1024, 640);
	set_position(Gtk::WIN_POS_CENTER);

	select_server_window.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	select_server_window.set_transient_for(*this);

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

#ifdef WIN32
	std::vector<Glib::RefPtr<Gdk::Pixbuf> > icon_list;
	icon_list.push_back( statusicon_pixbuf->scale_simple(16, 16, Gdk::INTERP_HYPER) );
	icon_list.push_back( statusicon_pixbuf->scale_simple(32, 32, Gdk::INTERP_HYPER) );
	icon_list.push_back( statusicon_pixbuf                                          );
	set_default_icon_list(icon_list);
#else
	set_default_icon(statusicon_pixbuf);
#endif

	statusicon->set_tooltip("gmpmpc\nA GTK+ Client for MPMP");
	statusicon->set_visible(true);

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
	main_paned.add(trackdb_widget);

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
}

void GtkMpmpClientWindow::on_menu_file_preferences() {
	dcerr("Prefs");
}

bool GtkMpmpClientWindow::on_delete_event(GdkEventAny* event) {
	on_statusicon_activate();
	return true;
}

void GtkMpmpClientWindow::on_menu_file_quit() {
	hide();
	select_server_window.hide();
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

	middle_end middleend;
	GtkMpmpClientWindow gmpmpc(middleend);
	Gtk::Main::run();
	gmpmpc.hide();
	return 0;
}

#ifndef DEBUG
#ifdef WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
					 int       nCmdShow)
{
	return makeErrorHandler(boost::bind(&main_impl, nCmdShow, &lpCmdLine))();
}
#endif
#endif

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}
