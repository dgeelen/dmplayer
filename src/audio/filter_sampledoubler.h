#ifndef FILTER_SAMPLEDOUBLER_H
#define FILTER_SAMPLEDOUBLER_H

#include "audiosource_interface.h"

class SampleDoublerFilter : public IAudioSource {
	private:
		IAudioSource* src;
	public:
		SampleDoublerFilter(IAudioSource* as);

		uint32 getData(uint8* buf, uint32 len);
};

#endif//FILTER_SAMPLEDOUBLER_H
