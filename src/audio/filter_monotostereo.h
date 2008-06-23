#ifndef FILTER_MONOTOSTEREO_H
#define FILTER_MONOTOSTEREO_H

#include "audiosource_interface.h"

class MonoToStereoFilter : public IAudioSource {
	private:
		IAudioSourceRef src;
	public:
		MonoToStereoFilter(IAudioSourceRef as);
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
};

#endif//FILTER_MONOTOSTEREO_H

