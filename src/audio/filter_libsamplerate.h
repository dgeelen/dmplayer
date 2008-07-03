#ifndef FILTER_LIBSAMPLERATE_H
#define FILTER_LIBSAMPLERATE_H

#include "audiosource_interface.h"
#include <samplerate.h>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>

class LibSamplerateFilter : public IAudioSource {
	public:
		typedef struct CALLBACK_DATA {
			SRC_STATE *src_state ;
			uint32 channels;
			uint32 buffer_begin;
			uint32 buffer_end;
			long input_buffer_index;
			std::vector<float> input_buffer;
			IAudioSourceRef src;
		};

		LibSamplerateFilter(IAudioSourceRef as, AudioFormat target);
		~LibSamplerateFilter();
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
	private:
		IAudioSourceRef src;
		CALLBACK_DATA callback_data;
};

#endif //FILTER_LIBSAMPLERATE_H
