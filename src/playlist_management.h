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

struct LocalTrack {
	LocalTrackID id;
	fspath filename;
	MetaDataMap metadata;

	LocalTrack(LocalTrackID id_, fspath filename_, MetaDataMap metadata_);
};

class Track {
public:
	TrackID id;
	MetaDataMap metadata;
	Track(TrackID id_, MetaDataMap metadata_);

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & id;
		ar & metadata;
	}
	Track() {};
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
//	private:
		std::vector<Track> entries;
};

#endif
