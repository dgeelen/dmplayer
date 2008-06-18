#include "gmpmpc_connection_handling.h"
#include "gmpmpc_select_server.h"
#include "gmpmpc_playlist.h"
#include "gmpmpc_trackdb.h"
#include <vector>
#include <glib.h>
using namespace std;

extern TreeviewPlaylist* treeview_playlist;
extern TrackDataBase trackDB;
extern network_handler* gmpmpc_network_handler;
ClientID gmpmpc_client_id;

void handle_received_message(const messageref m) {
	switch(m->get_type()) {
		case message::MSG_CONNECT: {
			dcerr("Received a MSG_CONNECT");
		} break;
		case message::MSG_ACCEPT: {
			dcerr("Received a MSG_ACCEPT");
			select_server_accepted();
			gmpmpc_client_id = boost::static_pointer_cast<message_accept>(m)->cid;
			gdk_threads_enter();
			treeview_trackdb_update(NULL);
			gdk_threads_leave();
		}; break;
		case message::MSG_DISCONNECT: {
			dcerr("Received a MSG_DISCONNECT");
			gmpmpc_network_handler->client_message_receive_signal.disconnect(handle_received_message);
		} break;
		case message::MSG_PLAYLIST_UPDATE: {
			dcerr("Received a MSG_PLAYLIST_UPDATE");
			message_playlist_update_ref msg = boost::static_pointer_cast<message_playlist_update>(m);
			msg->apply(treeview_playlist);
		}; break;
		case message::MSG_QUERY_TRACKDB: {
			dcerr("Received a MSG_QUERY_TRACKDB");
			message_query_trackdb_ref qr = boost::static_pointer_cast<message_query_trackdb>(m);
			vector<Track> res;
			BOOST_FOREACH(LocalTrack t, trackDB.search(qr->search)) {
				Track tr(TrackID(gmpmpc_client_id, t.id), t.metadata);
				res.push_back(tr);
			}
			message_query_trackdb_result_ref result(new message_query_trackdb_result(qr->qid, res));
			gmpmpc_network_handler->send_server_message(result);
			}; break;
		case message::MSG_QUERY_TRACKDB_RESULT: {
			dcerr("Received a MSG_QUERY_TRACKDB_RESULT");
		}; break;
		case message::MSG_REQUEST_FILE: {
			dcerr("Received a MSG_REQUEST_FILE");
		}; break;
		case message::MSG_REQUEST_FILE_RESULT: {
			dcerr("Received a MSG_REQUEST_FILE_RESULT");
		}; break;
		default: {
			dcerr("Ignoring unknown message of type " << m->get_type());
		} break;
	}
}
