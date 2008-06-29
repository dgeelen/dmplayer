#ifndef FILTER_LIBSAMPLERATE_h
#define FILTER_LIBSAMPLERATE_h

#include "audiosource_interface.h"
#include <samplerate.h>

class LibSamplerateFilter : public IAudioSource {
	private:
		IAudioSourceRef src;
		SRC_STATE* samplerate_converter;
		SRC_DATA   samplerate_converter_state;
	public:
		LibSamplerateFilter(IAudioSourceRef as, AudioFormat target);
		~LibSamplerateFilter();
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
};

#endif //FILTER_LIBSAMPLERATE_h
