#ifndef FILTER_DOWNMIX_H
#define FILTER_DOWNMIX_H

#include "audiosource_interface.h"
#include <boost/shared_ptr.hpp>

class DownmixFilter : public IAudioSource {
	public:
		DownmixFilter(IAudioSourceRef as, AudioFormat target);
		//~DownmixFilter();
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
	private:
		uint32 position;
		IAudioSourceRef src;
		AudioFormat target;
};

#endif //FILTER_DOWNMIX_H
