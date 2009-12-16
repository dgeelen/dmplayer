#include "playlist_management.h"
#include "error-handling.h"

#include "util/StrFormat.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/foreach.hpp>

namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
using namespace std;

std::ostream& operator<<(std::ostream& os, const TrackID& trackid) {
	return os << STRFORMAT("%08x:%08x", trackid.first, trackid.second);
}

/* Class LocalTrack */
LocalTrack::LocalTrack(LocalTrackID id_, fspath filename_, MetaDataMap metadata_)
:	id(id_)
,	filename(filename_)
,	metadata(metadata_)
{
}

/* Class Track */
Track::Track(TrackID id_, MetaDataMap metadata_)
:	id(id_)
,	metadata(metadata_)
{
}

/* Class TrackDataBase */
TrackDataBase::TrackDataBase() {

}

LocalTrackID TrackDataBase::get_first_free_id() {
	static LocalTrackID result(0);
	++result;
	return result;
	/* huh? how does this work? are entries ordered?
	BOOST_FOREACH(LocalTrack& t, entries) {
		if (result != t.id) break;
		++result;
	}
	return result;
	*/
}

vector<LocalTrack> TrackDataBase::search(Track track) {
	vector<LocalTrack> result;
	MetaDataMap& t = track.metadata;
	string filename = ba::to_lower_copy(t["FILENAME"]);
	BOOST_FOREACH(LocalTrack& tr, entries) {
		if (track.id.second != LocalTrackID(0xffffffff)) {
			if(track.id.second == tr.id) {
				result.push_back(tr);
			}
		}
		else if( ba::to_lower_copy(tr.metadata["FILENAME"]).find(filename) != string::npos ) {
			result.push_back(tr);
		}
	}
	return result;
}

void TrackDataBase::add_directory(fs::path path) {
	try {
		if(!fs::exists(path)) return;
	} catch(exception& e) { return; }

	if(fs::is_directory(path)) {
		fs::directory_iterator end_iter; // default construction yields past-the-end
		for(fs::directory_iterator iter(path);  iter != end_iter; ++iter ) {
			try {
				if(fs::exists(iter->path())) {
					add_directory(iter->path());
				}
			} catch(exception& e) {} // on error resume next (eg exception denied etc)
		}
	}
	else { // Some kind of file //FIXME: Handle recursive symlinks
		add(path);
	}
}

void TrackDataBase::add(fs::path path) {
	MetaDataMap meta_data;
	meta_data["FILENAME"] = path.leaf();
	entries.push_back(LocalTrack(get_first_free_id(), path, meta_data));
}

std::string Track::idstr() const {
	return STRFORMAT("%08x%08x", id.first, id.second);
}
