#ifndef FILTER_REFORMAT_H
#define FILTER_REFORMAT_H

#include "audiosource_interface.h"

class ReformatFilter : public IAudioSource {
	private:
		IAudioSourceRef src;
	public:
		ReformatFilter(IAudioSourceRef as, AudioFormat target);
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
};

#endif//FILTER_REFORMAT_H
