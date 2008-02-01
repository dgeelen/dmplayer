#include "playlist_management.h"
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

vector<SearchResult> TrackDataBase::search(Track t) {
	vector<SearchResult> result;
	for(vector<Track>::iterator i = entries.begin(); i!=entries.end(); ++i) {
		/* Todo: RegEx magix */
	}
	return result;
}
