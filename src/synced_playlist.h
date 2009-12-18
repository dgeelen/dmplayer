#ifndef SYNCED_PLAYLIST_H
#define SYNCED_PLAYLIST_H

#include "packet.h"
#include "error-handling.h"
#include "cross-platform.h"
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

class SyncedPlaylist : public IPlaylist {
	private:
		std::deque<message_playlist_update_ref> msgque;
		PlaylistVector data;
		mutable boost::mutex     internal_mutex;
		boost::condition sync_condition;

		bool playlist_sync_loop_running;
		boost::thread playlist_sync_loop_thread;
		void sync_loop() {
			while(playlist_sync_loop_running) {
				{
					boost::mutex::scoped_lock lock(internal_mutex);
					message_playlist_update_ref m;
					while(!msgque.empty()) {
						m = msgque.front();
						m->apply(data);
						sig_send_message(m);
						msgque.pop_front();
					}
					sync_condition.notify_one();
				}
				usleep(100*1000);
			}
		}

	public:
		/**
		 * Emitted whenever a message should be send to keep the playlist with
		 * it's counterpart(s).
		 * The callback should have the prototype <b>void(messageref)</b>
		 */
		boost::signal<void(messageref)> sig_send_message;

		/**
		 * Note: Don't forget to connect to SyncedPlaylist's sig_send_message.
		 *       This signal will be issued whenever a message needs to be send
		 *       via the network. It depends on your particular application
		 *       if you should use send_message_client(), send_message_clientall()
		 *       or something else entirely.
		 */
		SyncedPlaylist()
		: IPlaylist(),
		  playlist_sync_loop_running(true)
		{
			dcerr("Starting playlist_sync_loop_thread");
			boost::thread t1(makeErrorHandler(boost::bind(&SyncedPlaylist::sync_loop, this)));
			playlist_sync_loop_thread.swap(t1);
		}

		~SyncedPlaylist() {
			dcerr("Joining playlist_sync_loop_thread");
			playlist_sync_loop_running = false;
			playlist_sync_loop_thread.join();
		}

		/// clears all tracks from the playlist
		virtual void clear() {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update()));
		}

		/// adds given track to the end of the playlist
		virtual void append(const Track& track) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(track)));
		}

		/// appends a list of tracks at once to the playlist
		/// (can be used to increase performance incase add() is a slow operation)
		virtual void append(const std::vector<Track>& tracklist) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(tracklist)));
		}

		/// inserts given track at given position in the playlist
		virtual void insert(const uint32 pos, const Track& track) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(track, pos)));
		}

		/// inserts given tracks at given position in the playlist
		virtual void insert(const uint32 pos, const std::vector<Track>& tracklist) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(tracklist, pos)));
		}

		/// moves track from a given position to another one
		virtual void move(const uint32 from, const uint32 to) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(from, to)));
		}

		/// moves tracks from a given position to another one
		virtual void move(const std::vector<std::pair<uint32, uint32> >& to_from_list) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(to_from_list)));
		}

		/// removes track at given position from the playlist
		virtual void remove(const uint32 pos) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(pos)));
		}

		/// removes tracks at given positions from the playlist
		virtual void remove(const std::vector<uint32>& poslist) {
			boost::mutex::scoped_lock lock(internal_mutex);
			msgque.push_back(message_playlist_update_ref(new message_playlist_update(poslist)));
		}

		/// returns the track at the given position
		virtual const Track& get(const uint32 pos) const {
			boost::mutex::scoped_lock lock(internal_mutex);
			return data.get(pos);
		}

		/// returns the tracks at the given positions
		virtual const std::vector<Track> get(const std::vector<uint32>& poslist) const {
			assert(false); // FIXME: To be implemented.
			std::vector<Track> list;
			return list;
		}

		/// returns the number of tracks in the playlist
		virtual uint32 size() const {
			boost::mutex::scoped_lock lock(internal_mutex);
			return data.size();
		}

		/// Guarantee that all queued messages have been processed
		void sync() {
			boost::mutex::scoped_lock lock(internal_mutex);
			while(!msgque.empty()) {
				sync_condition.wait(lock);
			}
		}
};

#endif//SYNCED_PLAYLIST_H
