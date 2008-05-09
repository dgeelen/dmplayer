#ifndef PLAYLIST_MANAGEMENT_H
#define PLAYLIST_MANAGEMENT_H

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/strong_typedef.hpp>

//NOTE: Using an average of 5MB per song (reasonable for good quality MP3)
//      a uint32 should suffice to hold 20 pebibytes worth of music (2^32*(5*1024^2)/1024^5)

BOOST_STRONG_TYPEDEF(uint32, ClientID);
BOOST_STRONG_TYPEDEF(uint32, LocalTrackID);

typedef std::pair<ClientID, LocalTrackID> TrackID;

typedef std::map<std::string, std::string> MetaDataMap;

typedef boost::filesystem::path fspath;

//typedef std::pair<TrackID, MetaDataMap> SearchResult;

struct LocalTrack {
	LocalTrackID id;
	fspath filename;
	MetaDataMap metadata;

	LocalTrack(LocalTrackID id_, fspath filename_, MetaDataMap metadata_);
};

class Track {
	TrackID id;
	MetaDataMap metadata;
	Track(TrackID id_, MetaDataMap metadata_);
};

class TrackDataBase {
	public:
		TrackDataBase();
		void add(boost::filesystem::path path);
		void remove(LocalTrackID id);
		void add_directory(boost::filesystem::path path); //DEBUG or Utility?
		std::vector<LocalTrack> search(MetaDataMap t);
	private:
		LocalTrackID get_first_free_id();
		std::vector<LocalTrack> entries;
};

class Playlist {
	public:
		Playlist();
		void vote(TrackID id);
	private:
		std::vector<Track> entries;
};

#endif
