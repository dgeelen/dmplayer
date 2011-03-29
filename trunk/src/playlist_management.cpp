#include "playlist_management.h"
#include "error-handling.h"

#include "util/StrFormat.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>


#ifdef WIN32
	/**
	 * NOTE: When adding tracks we use boost::filesystem::exists() to check whether
	 *       a given path exists or not. However it appears that sometimes this can
	 *       cause a windows error popup to appear. The cause is that we may check
	 *       a non-existant driver for example, which causes windows to throw a
	 *       'Windows structured exception', not a C++ exception, and hence boost
	 *       can not catch it. See the following URL for someone with the same
	 *       problem: http://lists.boost.org/boost-users/2009/09/51934.php
	 *       Work around/solution: disable these type of errors.
	 */
	#include <windows.h>
	struct ErrorHider {
		ErrorHider() {
			SetErrorMode(SEM_FAILCRITICALERRORS);
		}
	};
	ErrorHider eh;
#endif

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

const boost::regex regex_match_no_filename("^/$"); // The forward slash is invalid in _file_names on both windows and linux
vector<LocalTrack> TrackDataBase::search(Track track) {
	vector<LocalTrack> result;
	MetaDataMap& t = track.metadata;
	string filename = ba::to_lower_copy(t["FILENAME"]);
	const char* md_match = "FILENAME";
	if(filename.substr(0,6) == "/path ") {
		filename = filename.substr(6);
		md_match = "PATH";
	}
	boost::regex regex(filename, boost::regex::perl|boost::regex::icase|boost::regex::no_except); // probably not every valid filename is also a valid or matching regex
	if(regex.status() != 0) { // regex is not valid
		regex = regex_match_no_filename;
	}
	vector<string> fn_parts;
	boost::split(fn_parts, filename, boost::is_space(), boost::token_compress_on);

	BOOST_FOREACH(LocalTrack& tr, entries) {
		if (track.id.second != LocalTrackID(0xffffffff)) {
			if(track.id.second == tr.id) {
				result.push_back(tr);
			}
		}
		else {
			const string& fn       = tr.metadata[md_match];
			const string  fn_lower = ba::to_lower_copy(fn);
			bool          matches  = true;
			BOOST_FOREACH(string& fn_part, fn_parts)
				matches = matches && (fn_lower.find(fn_part) != string::npos);
			if(matches || boost::regex_search(fn, regex))
				result.push_back(tr);
		}
	}
	return result;
}

bool my_exists(const fs::path& ph) {
	try {
		return fs::exists(ph);
	} catch (...) {
		return false;
	}
}

void TrackDataBase::add_directory(const fs::path& path) {
	add_directory_with_path(path.parent_path(), path);
}

void TrackDataBase::add_directory_with_path(const fs::path& base, const fs::path& path) {
	vector<fs::path> files;
	vector<fs::path> directories;
	if(my_exists(path)) {
		fs::directory_iterator end_iter; // default construction yields past-the-end
		for(fs::directory_iterator iter(path);  iter != end_iter; ++iter ) {
			if(fs::is_directory(iter->path()))
				directories.push_back(iter->path());
			else
				files.push_back(iter->path());
		}
	}


	std::sort(directories.begin(), directories.end());
	BOOST_FOREACH(const fs::path& directory, directories) {
		add_directory_with_path(base, directory);
	}

	std::sort(files.begin(), files.end());
	BOOST_FOREACH(const fs::path& file, files) {
		add_file_with_path(base, file);
	}
}

void TrackDataBase::add_file(const fs::path& path) {
	add_file_with_path(path.parent_path(), path);
}

/**
 * Adds a file to the trackdb, setting the "PATH" metadata such that it is
 * relative to base. This is to avoid giving away too much information
 * about the client's local filesystem layout.
 *
 * pre: (\exists p :: path = base / p)
 */
void TrackDataBase::add_file_with_path(const fs::path& base, const fs::path& path) {
	try {
		if(fs::is_regular(path)) {
			MetaDataMap meta_data;
			meta_data["FILENAME"] = path.leaf();

			fs::path relative_path;
			fs::path base_path = path.parent_path();
			while(!base.empty() && !base_path.empty() && base.parent_path() != base_path) {
				relative_path = base_path.leaf() / relative_path;
				base_path = base_path.parent_path();
			}

			meta_data["PATH"] = relative_path.string();
			entries.push_back(LocalTrack(get_first_free_id(), path, meta_data));
		}
	} catch(...) {}
}

std::string Track::idstr() const {
	return STRFORMAT("%08x%08x", id.first, id.second);
}
