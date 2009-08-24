#include "gmpmpc_trackdb.h"
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <vector>
#include <boost/bind.hpp>
#include "../util/StrFormat.h"
#include "../error-handling.h"

using namespace std;

gmpmpc_trackdb_widget::gmpmpc_trackdb_widget(middle_end& m)
: middleend(m),
  clientid(m.get_client_id())
{
	scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrolledwindow.add(treeview);
	search_hbox.pack_start(search_label, Gtk::PACK_SHRINK);
	search_vbox.add(search_hbox);
	search_vbox.add(search_entry);
	vbox.add(scrolledwindow);
	vbox.pack_start(search_vbox, Gtk::PACK_SHRINK);
	vbox.pack_start(add_to_wishlist_button, Gtk::PACK_SHRINK);
	add(vbox);

	set_label("TrackDB:");
	search_label.set_label("Filter:");
	add_to_wishlist_button.set_border_width(3);
	add_to_wishlist_button.set_label("Enqueue selected");
// 	focus_add_to_wishlist_button();
	add_to_wishlist_button.grab_focus();

	std::list<Gtk::TargetEntry> listTargets;
	listTargets.push_back( Gtk::TargetEntry("text/uri-list", Gtk::TARGET_OTHER_APP, 0) ); // Last parameter is 'Info', used to distinguish
	listTargets.push_back( Gtk::TargetEntry("text/plain"   , Gtk::TARGET_OTHER_APP, 1) ); // different types if TargetEntry in the drop handler
	listTargets.push_back( Gtk::TargetEntry("STRING"       , Gtk::TARGET_OTHER_APP, 1) );

	treeview.drag_dest_set(listTargets); // Should use defaults, DEST_DEFAULT_ALL, Gdk::ACTION_COPY);

	search_entry.signal_changed().connect(boost::bind(&gmpmpc_trackdb_widget::on_search_entry_changed, this));
	add_to_wishlist_button.signal_clicked().connect(boost::bind(&gmpmpc_trackdb_widget::on_add_to_wishlist_button_clicked, this));
	treeview.signal_drag_data_received().connect(sigc::mem_fun(*this, &gmpmpc_trackdb_widget::on_drag_data_received_signal));
// 	treeview.add_events(Gdk::BUTTON_PRESS_MASK);
// 	treeview.signal_button_press_event().connect(sigc::mem_fun(*this, &gmpmpc_trackdb_widget::on_treeview_clicked), false);

	sig_search_tracks_connection =
		middleend.sig_search_tracks.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_trackdb_widget::sig_search_tracks_handler, this, _1, _2)));
	update_treeview_connection =
		middleend.sig_client_id_changed.connect(
			dispatcher.wrap(boost::bind(&gmpmpc_trackdb_widget::update_treeview, this)));

	update_treeview();
// 	#ifdef DEBUG
// 	network_search_id=0; //Intentionally not initialized
// 	#endif
}

gmpmpc_trackdb_widget::~gmpmpc_trackdb_widget() {
	update_treeview_connection.disconnect();
	sig_search_tracks_connection.disconnect();
	search_entry_timeout_connection.disconnect();
	//FIXME: Should we also disconnect search_entry and others
}

void gmpmpc_trackdb_widget::set_clientid(ClientID id) {
	clientid = id;
	update_treeview();
}

// bool gmpmpc_trackdb_widget::focus_add_to_wishlist_button() {
// 	add_to_wishlist_button.grab_focus();
// 	return false;
// }

// bool gmpmpc_trackdb_widget::on_treeview_clicked(GdkEventButton *event) {
// 	Glib::signal_idle().connect(sigc::mem_fun(*this, &gmpmpc_trackdb_widget::focus_add_to_wishlist_button));
// 	return false;
// }

void gmpmpc_trackdb_widget::on_add_to_wishlist_button_clicked() {
	Glib::RefPtr<Gtk::TreeModel> model = treeview.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	status_message_signal("Appending track...");
	Glib::RefPtr<Gtk::TreeSelection> sel = treeview.get_selection();
	std::vector<Gtk::TreeModel::Path> selected_rows = sel->get_selected_rows();
	BOOST_FOREACH(Gtk::TreeModel::Path p, selected_rows) {
		Gtk::TreeModel::iterator i = model->get_iter(p);
		middleend.mylist_append((*i)[treeview.m_Columns.track]);
// 		Track q;
// 		q = (*i)[treeview.m_Columns.track];
// 		std::vector<LocalTrack> r;// = trackdb.search(q);
// 		Track t(q.id, r[0].metadata);
// 		enqueue_track_signal(q);
	}
}

void gmpmpc_trackdb_widget::update_treeview() {
	search_entry_timeout_handler();
}

bool gmpmpc_trackdb_widget::search_entry_timeout_handler() {
	MetaDataMap m;
	m["FILENAME"] = search_entry.get_text();
	Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), m);
	treeview.clear();
	search_id = middleend.search_tracks(query);
	return false;
}

void gmpmpc_trackdb_widget::sig_search_tracks_handler(const SearchID id, const std::vector<Track>& tracklist) {
	if(id == search_id) {
		treeview.batch_add(tracklist);
	}
}

void gmpmpc_trackdb_widget::on_search_entry_changed() {
	search_entry_timeout_connection.disconnect();
	sigc::slot<bool> slot = sigc::mem_fun(*this, &gmpmpc_trackdb_widget::search_entry_timeout_handler);
	search_entry_timeout_connection = Glib::signal_timeout().connect(slot, 333);
}

string urldecode(std::string s) { //http://www.koders.com/cpp/fid6315325A03C89DEB1E28732308D70D1312AB17DD.aspx
	std::string buffer = "";
	int len = s.length();

	for (int i = 0; i < len; i++) {
		int j = i ;
		char ch = s.at(j);
		if (ch == '%'){
			char tmpstr[] = "0x0__";
			int chnum;
			tmpstr[3] = s.at(j+1);
			tmpstr[4] = s.at(j+2);
			chnum = strtol(tmpstr, NULL, 16);
			buffer += chnum;
			i += 2;
		} else {
			buffer += ch;
		}
	}
	return buffer;
}

vector<std::string> urilist_convert(const std::string urilist) {
	vector<std::string> files;
	int begin = urilist.find("file://", 0);
	int end = urilist.find("\r\n", begin);
	while( end != string::npos) {
		files.push_back(urldecode(urilist.substr(begin + 7, end-begin-7)));
		begin = urilist.find("file://", end);
		if(begin == string::npos) break;
		end = urilist.find("\r\n", begin);
	}
	return files;
}

void gmpmpc_trackdb_widget::on_drag_data_received_signal(const Glib::RefPtr<Gdk::DragContext>& context,
                                                         int x, int y,
                                                         const Gtk::SelectionData& selection_data,
                                                         guint info, guint time) {
	dcerr("DROP!");

	// Note: We know we only receive strings (of type STRING, text/uri-list and text/plain)
	switch(info) {
		case 0: {
			BOOST_FOREACH(std::string file, urilist_convert(selection_data.get_data_as_string())) {
// 				trackdb.add_directory(file);
			}
		}; break;
		case 1: {
// 			trackdb.add(boost::filesystem::path(selection_data.get_data_as_string()));
		}; break;
		default:
			dcerr("Unhandled drop? info="<<info);
	}
	update_treeview();
	context->drop_finish(true, time);
}
