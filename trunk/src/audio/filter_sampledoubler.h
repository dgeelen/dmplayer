#ifndef FILTER_SAMPLEDOUBLER_H
#define FILTER_SAMPLEDOUBLER_H

#include "audiosource_interface.h"

class SampleDoublerFilter : public IAudioSource {
	private:
		IAudioSourceRef src;
	public:
		SampleDoublerFilter(IAudioSourceRef as);

		uint32 getData(uint8* buf, uint32 len);
};

#endif//FILTER_SAMPLEDOUBLER_H
