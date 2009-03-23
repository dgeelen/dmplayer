#include "gmpmpc_playlist.h"
#include <boost/bind.hpp>

gmpmpc_playlist_widget::gmpmpc_playlist_widget() {
	scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrolledwindow.add(treeview);
	vbox.add(scrolledwindow);
	vbox.pack_start(vote_min_button, Gtk::PACK_SHRINK);
	Gtk::Frame::add(vbox);
	set_label("Playlist:");
	vote_min_button.set_border_width(3);
	vote_min_button.set_label("Vote MIN");
	vote_min_button.signal_clicked().connect(boost::bind(&gmpmpc_playlist_widget::on_vote_min_button_clicked, this));
}

void gmpmpc_playlist_widget::update(message_playlist_update_ref m) {
	m->apply(&treeview);
}

void gmpmpc_playlist_widget::add_to_wishlist(Track& track) {
	wish_list.add(track);
	messageref msg;

	while (msg = wish_list.pop_msg()) {
		send_message_signal(msg);
	}
}

void gmpmpc_playlist_widget::on_vote_min_button_clicked() {
	Glib::RefPtr<Gtk::TreeModel> model = treeview.get_model();
	Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(model);

	Glib::RefPtr<Gtk::TreeSelection> sel = treeview.get_selection();
	std::vector<Gtk::TreeModel::Path> selected_rows = sel->get_selected_rows();
	BOOST_FOREACH(Gtk::TreeModel::Path p, selected_rows) {
		Gtk::TreeModel::iterator i = model->get_iter(p);
		Track t = (*i)[treeview.m_Columns.track];
		vote_signal(t.id, -1);
	}
}
