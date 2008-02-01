#include "playlist_management.h"
#include "error-handling.h"
#include "boost/algorithm/string/case_conv.hpp"

namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
using namespace std;

/* Class Track */
Track::Track() {
	id = -1;
}

Track::Track(uint32 id, string filename, map<string, string> meta_data) {
	this->id = id;
	this->filename = filename;
	this->meta_data = meta_data;
}

/* Class TrackDataBase */
TrackDataBase::TrackDataBase() {

}

uint32 TrackDataBase::get_first_free_id() {
	uint32 result = 0;
	for(vector<Track>::iterator i = entries.begin(); i!=entries.end(); ++i) {
		if(result != i->id) break;
		++result;
	}
	return result;
}

void TrackDataBase::add(Track t) {
	t.id = get_first_free_id();
	entries.push_back(t);
}

vector<Track> TrackDataBase::search(Track t) {
	vector<Track> result;
	string filename = ba::to_lower_copy(t.meta_data["FILENAME"]);
	for(vector<Track>::iterator i = entries.begin(); i!=entries.end(); ++i) {
		if( ba::to_lower_copy(i->meta_data["FILENAME"]).find(filename, -1) != -1 ) {
			result.push_back(*i);
		}
	}
	return result;
}

void TrackDataBase::add_directory(fs::path path) {
	if(!fs::exists(path)) return;
	if(fs::is_directory(path)) {
		fs::directory_iterator end_iter; // default construction yields past-the-end
		for(fs::directory_iterator iter(path);  iter != end_iter; ++iter ) {
			if(fs::exists(iter->path())) {
				add_directory(iter->path());
			}
		}
	}
	else { // Some kind of file
		map<string, string> meta_data;
		meta_data["FILENAME"] = path.leaf();
		add(Track(get_first_free_id(), path.string(), meta_data ));
	}
}

