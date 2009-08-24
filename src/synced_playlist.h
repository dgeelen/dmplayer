#ifndef SYNCED_PLAYLIST_H
#define SYNCED_PLAYLIST_H

#include "packet.h"

class SyncedPlaylist : public IPlaylist {
	private:
		std::deque<message_playlist_update_ref> msgque;
		PlaylistVector data;
		mutable boost::mutex internal_mutex;
		//virtual void vote(TrackID id) { Playlist::vote(id); };
		//virtual void add(Track track)  { Playlist::add(track); };
		//virtual void clear()  { Playlist::clear(); };
	public:
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

		messageref pop_msg() {
			boost::mutex::scoped_lock lock(internal_mutex);
			message_playlist_update_ref ret;
			if (!msgque.empty()) {
				ret = msgque.front();
				ret->apply(data);
				msgque.pop_front();
			}

			return ret;
		}
};

#endif//SYNCED_PLAYLIST_H
