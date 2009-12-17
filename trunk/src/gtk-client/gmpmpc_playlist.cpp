#include "gmpmpc_playlist.h"
#include <boost/bind.hpp>

gmpmpc_playlist_widget::gmpmpc_playlist_widget(middle_end& m)
: middleend(m)
{
	treeview = IPlaylistRef(new gmpmpc_track_treeview());
	scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrolledwindow.add(*(gmpmpc_track_treeview*)treeview.get());
	vbox.add(scrolledwindow);
	vbox.pack_start(vote_down_button, Gtk::PACK_SHRINK);
	Gtk::Frame::add(vbox);
	set_label("Playlist:");
	vote_down_button.set_border_width(3);
	vote_down_button.set_use_underline(true);
	vote_down_button.set_label("Vote _DOWN");
	vote_down_button.signal_clicked().connect(boost::bind(&gmpmpc_playlist_widget::on_vote_down_button_clicked, this));

	sig_update_playlist_connection =
		middleend.sig_update_playlist.connect(
			boost::bind(&gmpmpc_playlist_widget::sig_update_playlist_handler, this));
}

gmpmpc_playlist_widget::~gmpmpc_playlist_widget() {
	sig_update_playlist_connection.disconnect();
}

/* This function may be called asynchronously, so be carefull when using GTK here */
IPlaylistRef gmpmpc_playlist_widget::sig_update_playlist_handler() {
	return treeview;
}

void gmpmpc_playlist_widget::add_to_wishlist(Track& track) {
	wish_list.append(track);
}

void gmpmpc_playlist_widget::on_vote_down_button_clicked() {
	Glib::RefPtr<Gtk::TreeModel> model = ((gmpmpc_track_treeview*)treeview.get())->get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	Glib::RefPtr<Gtk::TreeSelection> sel = ((gmpmpc_track_treeview*)treeview.get())->get_selection();
	std::vector<Gtk::TreeModel::Path> selected_rows = sel->get_selected_rows();
	BOOST_FOREACH(Gtk::TreeModel::Path p, selected_rows) {
		Gtk::TreeModel::iterator i = model->get_iter(p);
		Track t = (*i)[((gmpmpc_track_treeview*)treeview.get())->m_Columns.track];
		middleend.playlist_vote_down(t);
// 		vote_signal(t.id, -1);
	}
}
