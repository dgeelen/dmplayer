#include "filter_split.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <boost/thread/mutex.hpp>

using namespace std;

SplitFilter::SplitFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	index_1 = 0;
	index_2 = 0;
}

SplitFilter::~SplitFilter() {
// 		if(read < toread) {
// 			if(src->exhausted() && !is_exhausted) {
// 				is_exhausted = true;
// 			}
// 		}
}

uint32 SplitFilter::getData(uint8* buf, uint32 len) {
	boost::mutex::scoped_lock lock(mutex);
	if(buffer.size() - index_1 < len) {
		uint32 toread = len - (buffer.size() - index_1);
		vector<uint8> b(toread);
		uint32 read = src->getData(&b[0], toread);
		buffer.insert(buffer.end(), b.begin(), b.begin()+read);
	}
	uint32 rlen = std::min(len, buffer.size() - index_1);
	std::copy(buffer.begin() + index_1, buffer.begin() + (index_1+rlen), &buf[0]);
	index_1 += rlen;
	if(index_1 >= index_2) {
		buffer.erase(buffer.begin(), buffer.begin() + index_2);
		index_1 -= index_2;
		index_2  = 0;
	}
	return rlen;
}

uint32 SplitFilter::getData2(uint8* buf, uint32 len) {
	boost::mutex::scoped_lock lock(mutex);
	if(buffer.size() - index_2 < len) {
		uint32 toread = len - (buffer.size() - index_2);
		vector<uint8> b(toread);
		uint32 read = src->getData(&b[0], toread);
		buffer.insert(buffer.end(), b.begin(), b.begin()+read);
	}
	uint32 rlen = std::min(len, buffer.size() - index_2);
	std::copy(buffer.begin() + index_2, buffer.begin() + (index_2+rlen), &buf[0]);
	index_2 += rlen;
	if(index_2 >= index_1) {
		buffer.erase(buffer.begin(), buffer.begin() + index_1);
		index_2 -= index_1;
		index_1  = 0;
	}
	return rlen;
}

bool SplitFilter::exhausted() {
	return (buffer.size() == index_1) && src->exhausted();
}

bool SplitFilter::exhausted2() {
	return (buffer.size() == index_2) && src->exhausted();
}
