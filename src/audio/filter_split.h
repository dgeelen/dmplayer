#ifndef FILTER_SPLIT_H
#define FILTER_SPLIT_H

#include "audiosource_interface.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <deque>
#include <vector>

// The split filter reproduces the input stream on it's two outputs
class SplitFilter : public IAudioSource {
	public:
		SplitFilter(IAudioSourceRef as, AudioFormat target);
		~SplitFilter();
		bool exhausted();
		bool exhausted2();
		uint32 getData(uint8* buf, uint32 len);
		uint32 getData2(uint8* buf, uint32 len);
	private:
		IAudioSourceRef src;
		boost::mutex mutex;
		uint32 index_1;
		uint32 index_2;
		std::deque<uint8> buffer;
};

#endif //FILTER_SPLIT_H
