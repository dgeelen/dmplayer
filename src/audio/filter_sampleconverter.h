#ifndef FILTER_SAMPLECONVERTER_H
#define FILTER_SAMPLECONVERTER_H

#include "audiosource_interface.h"

class SampleConverterFilter : public IAudioSource {
	private:
		uint32 fill_buffer(uint8* buffer, uint32 count, uint32 size_of);
		uint32 convert_int8_to_float(float* output, uint32 size);
		uint32 convert_uint8_to_float(float* output, uint32 size);
		uint32 convert_int16_to_float(float* output, uint32 size);
		uint32 convert_float_to_int8(int8* output, uint32 size);
		uint32 convert_float_to_int16(int16* output, uint32 size);
		IAudioSourceRef src;
	public:
		SampleConverterFilter(IAudioSourceRef as, AudioFormat target);
		~SampleConverterFilter();
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
};

#endif //FILTER_SAMPLECONVERTER_H
