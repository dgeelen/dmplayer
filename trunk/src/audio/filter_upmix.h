#ifndef FILTER_UPMIX_H
#define FILTER_UPMIX_H

#include "audiosource_interface.h"
#include "filter_iir.h"
#include "filter_split.h"
#include <boost/shared_ptr.hpp>
#include <boost/circular_buffer.hpp>
#include <deque>
#include <vector>
#include <set>

class UpmixFilter : public IAudioSource {
	public:
		UpmixFilter(IAudioSourceRef as, AudioFormat target);
		//~UpmixFilter();
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
	private:
		uint32 position;
		IAudioSourceRef src;
		AudioFormat target;
};

#endif //FILTER_UPMIX_H
