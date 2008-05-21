#include "playlist_management.h"
#include "error-handling.h"
#include "boost/algorithm/string/case_conv.hpp"
#include <boost/foreach.hpp>

namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
using namespace std;

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

vector<LocalTrack> TrackDataBase::search(MetaDataMap t) {
	vector<LocalTrack> result;
	string filename = ba::to_lower_copy(t["FILENAME"]);
	BOOST_FOREACH(LocalTrack& tr, entries) {
		if( ba::to_lower_copy(tr.metadata["FILENAME"]).find(filename, -1) != -1 ) {
			result.push_back(tr);
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
	else { // Some kind of file //FIXME: Handle recursive symlinks
		add(path);
	}
}

void TrackDataBase::add(fs::path path) {
	MetaDataMap meta_data;
	meta_data["FILENAME"] = path.leaf();
	entries.push_back(LocalTrack(get_first_free_id(), path, meta_data));
}
