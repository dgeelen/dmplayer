#ifndef PLAYLIST_MANAGEMENT_H
#define PLAYLIST_MANAGEMENT_H

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/strong_typedef.hpp>

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
		/// adds given track to the end of the playlist
		virtual void add(const Track& track) = 0;

		/// removes track at given position from the playlist
		virtual void remove(uint32 pos) = 0;

		/// inserts given track at given position in the playlist
		virtual void insert(uint32 pos, const Track& track) = 0;

		/// moves track from a given position to another one
		virtual void move(uint32 from, uint32 to) = 0;

		/// clears all tracks from the playlist
		virtual void clear() = 0;

		/// returns the number of tracks in the playlist
		virtual uint32 size() const = 0;

		/// returns the track at the given position
		virtual const Track& get(uint32 pos) const = 0;

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

class PlaylistVector : public IPlaylist {
	private:
		std::vector<Track> data;
	public:
		virtual void add(const Track& track) {
			data.push_back(track);
		}

		virtual void remove(uint32 pos) {
			data.erase(data.begin()+pos);
		}

		virtual void insert(uint32 pos, const Track& track) {
			data.insert(data.begin()+pos, track);
		}

		virtual void move(uint32 from, uint32 to) {
			Track t = get(from);
			if (from <= to) {
				insert(to, t);
				remove(from);
			} else {
				remove(from);
				insert(to, t);
			}
		}

		virtual void clear() {
			data.clear();
		}

		virtual uint32 size() const {
			return data.size();
		}

		virtual const Track& get(uint32 pos) const {
			return data[pos];
		}
};

#endif
