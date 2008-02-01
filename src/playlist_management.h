#ifndef PLAYLIST_MANAGEMENT_H
	#define PLAYLIST_MANAGEMENT_H
	#include "types.h"
	#include <string>
	#include <vector>
	#include <map>

	typedef std::pair<uint32,uint32> TrackID;

	typedef std::pair<TrackID, std::map<std::string, std::string> > SearchResult;

	struct Track {
			Track();
			Track(uint32 id, std::string filename, std::map<std::string, std::string> meta_data);
			uint32 id;
			std::string filename;
			std::map<std::string, std::string> meta_data;
	};

	class TrackDataBase {
		public:
			TrackDataBase();
			void add(Track t);
			void add_directory(std::string path); //DEBUG or Utility?
			void remove(Track t);
			void remove(uint32 id);
			std::vector<SearchResult> search(Track t);
		private:
			uint32 get_first_free_id();
			std::vector<Track> entries;
	};

	class Playlist {
		public:
			Playlist();
			void vote(TrackID id);
		private:
			std::vector<TrackID> entries;
	};

#endif
