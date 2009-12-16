#ifndef PLAYLIST_MANAGEMENT_H
#define PLAYLIST_MANAGEMENT_H

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/strong_typedef.hpp>
#include <boost/foreach.hpp>

#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>

//NOTE: Using an average of 5MB per song (reasonable for good quality MP3)
//      a uint32 should suffice to hold 20 pebibytes worth of music (2^32*(5*1024^2)/1024^5)

BOOST_STRONG_TYPEDEF(uint32, ClientID);
BOOST_STRONG_TYPEDEF(uint32, LocalTrackID);

template<class Archive>
void serialize(
	Archive &ar,
	ClientID & i,
	const unsigned version
){
	ar & i.t;
}

template<class Archive>
void serialize(
	Archive &ar,
	LocalTrackID & i,
	const unsigned version
){
	ar & i.t;
}

typedef std::pair<ClientID, LocalTrackID> TrackID;
std::ostream& operator<<(std::ostream& os, const TrackID& trackid);

typedef std::map<std::string, std::string> MetaDataMap;

typedef boost::filesystem::path fspath;

//typedef std::pair<TrackID, MetaDataMap> SearchResult;

class Track {
public:
	TrackID id;
	MetaDataMap metadata;
	Track(TrackID id_, MetaDataMap metadata_);

	std::string idstr() const;

	Track() {
		id = TrackID(ClientID(-1), LocalTrackID(-1));
	};

	bool operator==(const Track& that) const {
		return this->id == that.id;
	}

	bool operator!=(const Track& that) const {
		return this->id != that.id;
	}

	bool operator<(const Track& that) const { // Needed for insertion into std::map
		return this->id < that.id;
	}
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & id;
		ar & metadata;
	}
};

struct LocalTrack {
	LocalTrackID id;
	fspath filename;
	MetaDataMap metadata;

	LocalTrack(LocalTrackID id_, fspath filename_, MetaDataMap metadata_);

	Track getTrack() {
		return Track(TrackID(ClientID(-1),id), metadata);
	}
};


class TrackDataBase {
	public:
		TrackDataBase();
		void add(boost::filesystem::path path);
		void remove(LocalTrackID id);
		void add_directory(boost::filesystem::path path); //DEBUG or Utility?
		std::vector<LocalTrack> search(Track t);
	private:
		LocalTrackID get_first_free_id();
		std::vector<LocalTrack> entries;
};

class IPlaylist {
	public:
		/// clears all tracks from the playlist
		virtual void clear() = 0;

		/// adds given track to the end of the playlist
		virtual void append(const Track& track) = 0;

		/// appends a list of tracks at once to the playlist
		virtual void append(const std::vector<Track>& tracklist) = 0;

		/// inserts given track at given position in the playlist
		virtual void insert(const uint32 pos, const Track& track) = 0;

		/// inserts given tracks at given position in the playlist
		virtual void insert(const uint32 pos, const std::vector<Track>& track) = 0;

		/// moves track from a given position to another one
		virtual void move(const uint32 from, const uint32 to) = 0;

		/// moves tracks from given positions to other positions
		virtual void move(const std::vector<std::pair<uint32, uint32> >& from_to_list) = 0;

		/// removes track at given position from the playlist
		virtual void remove(const uint32 pos) = 0;

		/// removes tracks at given positions from the playlist
		virtual void remove(const std::vector<uint32>& poslist) = 0;

		/// returns the track at the given position
		virtual const Track& get(const uint32 pos) const = 0;

		/// returns the tracks at the given positions
		virtual const std::vector<Track> get(const std::vector<uint32>& positions) const = 0;

		/// returns the number of tracks in the playlist
		virtual uint32 size() const = 0;

		/// here comes iterator magix
		struct const_iterator {
			const_iterator() : playlist(NULL), pos(0) {};
			const_iterator(const class IPlaylist* playlist_, int32 pos_) : playlist(playlist_), pos(pos_) {};
			const class IPlaylist* playlist;
			int32 pos;
			const Track& operator*() { return playlist->get(pos); };
			typedef std::random_access_iterator_tag iterator_category;
			typedef int32 difference_type;
			typedef const Track value_type;
			typedef const Track* pointer;
			typedef const Track& reference;
			bool operator==(const const_iterator& o) const { return (playlist == o.playlist) && (pos == o.pos); };
			const_iterator operator++() { ++pos; return *this; };
		};

		const_iterator begin() const { return const_iterator(this, 0); };
		const_iterator end() const { return const_iterator(this, size()); };
};
typedef boost::shared_ptr<IPlaylist> IPlaylistRef;

class PlaylistVector : public IPlaylist {
	private:
		std::vector<Track> data;
	public:
		class iterator : public std::iterator<std::forward_iterator_tag, Track> {
			public:
				iterator() : _iterator() {};
				iterator(const std::vector<Track>::iterator& i) : _iterator(i) {};
				bool operator==(const iterator& that) const {
					return this->_iterator == that._iterator;
				};
				bool operator!=(const iterator& that) const {
					return this->_iterator != that._iterator;
				};
				iterator operator++() {
					this->_iterator++;
					return *this;
				}
				Track& operator*() {
					return *_iterator;
				}
			private:
				std::vector<Track>::iterator _iterator;
		};
		iterator begin() { return iterator(data.begin()); }
		iterator end() { return iterator(data.end()); }

		virtual void clear() {
			data.clear();
		}

		virtual void append(const Track& track) {
			data.push_back(track);
		}

		virtual void append(const std::vector<Track>& tracklist) {
			data.insert(data.end(), tracklist.begin(), tracklist.end());
			////FIXME: Proper append to data
			//BOOST_FOREACH(const Track& track, tracklist) {
				//data.push_back(track);
			//}
		}

		virtual void insert(const uint32 pos, const Track& track) {
			data.insert(data.begin()+pos, track);
		}

		virtual void insert(const uint32 pos, const std::vector<Track>& tracklist) {
			assert(false); // FIXME: To be implemented.
			//data.insert(data.begin()+pos, track);
		}

		virtual void move(const uint32 from, const uint32 to) {
			Track t = get(from);
			if (from <= to) {
				insert(to, t);
				remove(from);
			} else {
				remove(from);
				insert(to, t);
			}
		}

		virtual void move(const std::vector<std::pair<uint32, uint32> >& to_from_list) {
			assert(false); // FIXME: To be implemented.
		}

		virtual void remove(const uint32 pos) {
			data.erase(data.begin()+pos);
		}

		virtual void remove(const std::vector<uint32>& poslist) {
			assert(false); // FIXME: To be implemented.
		}

		virtual const Track& get(const uint32 pos) const {
			return data[pos];
		}

		virtual const std::vector<Track> get(const std::vector<uint32>& poslist) const {
			assert(false); // FIXME: To be implemented.
			std::vector<Track> list;
			return list;
		}

		virtual uint32 size() const {
			return data.size();
		}
};

#endif
