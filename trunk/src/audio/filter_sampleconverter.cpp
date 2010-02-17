#include "filter_sampleconverter.h"
#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <algorithm>

//FIXME: Put this somewhere safe
std::ostream& operator<<(std::ostream& os, const AudioFormat& af) {
	return os << "SampleRate="    << af.SampleRate    << ", "
						<< "Channels="      << af.Channels      << ", "
						<< "BitsPerSample=" << af.BitsPerSample << ", "
						<< "SignedSample="  << af.SignedSample  << ", "
						<< "LittleEndian="  << af.LittleEndian  << ", "
						<< "Float="         << af.Float;
}

using namespace std;

SampleConverterFilter::SampleConverterFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	if(
	(audioformat.BitsPerSample == target.BitsPerSample) &&
	(audioformat.Float         == target.Float) &&
	(audioformat.SignedSample  == target.SignedSample) &&
	(audioformat.LittleEndian  == target.LittleEndian)
	) {
		dcerr("Cannot convert sampletype [" << src->getAudioFormat() << "] to [" << audioformat << "]");
		throw Exception("Cannot convert sampletype");
	}
	audioformat.BitsPerSample = target.BitsPerSample;
	audioformat.Float         = target.Float;
	audioformat.SignedSample  = target.SignedSample;
	audioformat.LittleEndian  = target.LittleEndian;
	dcerr("Conversion: [" << src->getAudioFormat() << "] to [" << audioformat << "]");
}

SampleConverterFilter::~SampleConverterFilter() {
	dcerr("shutting down");
}

uint32 SampleConverterFilter::fill_buffer(uint8* buffer, uint32 count, uint32 size_of) {
	uint32 todo = count * size_of;
	uint32 size = todo;
	while(todo) {
		int read = src->getData(buffer + size - todo, todo);
		todo -= read;
		if(read == 0 && src->exhausted()) {
			return ((count * size_of) - todo) / size_of;
		}
	}
	return count;
}

uint32 SampleConverterFilter::convert_int8_to_float(float* output, uint32 size) {
	int8* input = new int8[size];

	size = fill_buffer((uint8*)input, size, sizeof(int8));

	for(uint32 i = 0; i < size; ++i) {
		output[i] = float(input[i]) / 128.0f;
	}

	delete[] input;
	return size * sizeof(float);
}

uint32 SampleConverterFilter::convert_uint8_to_float(float* output, uint32 size) {
	uint8* input = new uint8[size];

	size = fill_buffer((uint8*)input, size, sizeof(uint8));

	for(uint32 i = 0; i < size; ++i) {
		output[i] = float(input[i]) / 128.0f - 1.0f;
	}

	delete[] input;
	return size * sizeof(float);
}

uint32 SampleConverterFilter::convert_int16_to_float(float* output, uint32 size) {
	int16* input = new int16[size];

	size = fill_buffer((uint8*)input, size, sizeof(int16));

	for(uint32 i = 0; i < size; ++i) {
		output[i] = float(input[i]) / 32768.0f;
	}

	delete[] input;
	return size * sizeof(float);
}

uint32 SampleConverterFilter::convert_uint16_to_float(float* output, uint32 size) {
	uint16* input = new uint16[size];

	size = fill_buffer((uint8*)input, size, sizeof(uint16));

	for(uint32 i = 0; i < size; ++i) {
		output[i] = float(input[i]) / 32768.0f - 1.0f;
	}

	delete[] input;
	return size * sizeof(float);
}

uint32 SampleConverterFilter::convert_float_to_int8(int8* output, uint32 size) {
	float* input = new float[size];

	size = fill_buffer((uint8*)input, size, sizeof(float));

	for(uint32 i = 0; i < size; ++i) {
		float f = input[i];
		f = (f>1.0f) ? 1.0f : ((f<-1.0f) ? -1.0f : f); // Clip to -1 .. 1
		output[i] = int8(f * 127.0f);
	}

	delete[] input;
	return size * sizeof(int8);
}

uint32 SampleConverterFilter::convert_float_to_int16(int16* output, uint32 size) {
	float* input = new float[size];

	size = fill_buffer((uint8*)input, size, sizeof(float));

	for(uint32 i = 0; i < size; ++i) {
		float f = input[i];
		f = (f>1.0f) ? 1.0f : ((f<-1.0f) ? -1.0f : f); // Clip to -1 .. 1
		output[i] = int16(f * 32767.0f);
	}

	delete[] input;
	return size * sizeof(int16);
}


/**
 * Note:
 * We assume requests come in multiples of the sample size,
 * for example multiples of 4 bytes in the case of audioformat.Channels * sizeof(TargetType)
 * with audioformat.Channels == 2 and TargetType == Unsigned 16Bit
 **/
uint32 SampleConverterFilter::getData(uint8* buf, uint32 len) {
	if(src->getAudioFormat().LittleEndian != audioformat.LittleEndian) {
		dcerr("Cannot convert sampletype [" << src->getAudioFormat() << "] to [" << audioformat << "]");
		throw Exception("Endianness conversions are not implemented (yet)");
	}
	if(src->getAudioFormat().Float != audioformat.Float) {
		if(src->getAudioFormat().Float) {
			switch(audioformat.BitsPerSample) {
				case 8: {
					if(audioformat.SignedSample) {
						return convert_float_to_int8((int8*)buf, len / sizeof(int8));
					}
				}; break;
				case 16: {
					if(audioformat.SignedSample) {
						return convert_float_to_int16((int16*)buf, len / sizeof(int16));
					}
				}; break;
				case 32: {
				}; break;
			}
		}
		else { // audioformat.Float
			switch(src->getAudioFormat().BitsPerSample) {
				case 8: {
					if(src->getAudioFormat().SignedSample)
						return convert_int8_to_float((float*)buf, len / sizeof(float));
					else
						return convert_uint8_to_float((float*)buf, len / sizeof(float));
				}; break;
				case 16: {
					if(src->getAudioFormat().SignedSample)
						return convert_int16_to_float((float*)buf, len / sizeof(float));
					else
						return convert_uint16_to_float((float*)buf, len / sizeof(float));
				}; break;
				case 32: {
				}; break;
			}
		}
	}
	dcerr("Cannot convert sampletype [" << src->getAudioFormat() << "] to [" << audioformat << "]");
	throw Exception("Cannot convert sampletype");
}

bool SampleConverterFilter::exhausted() {
	return src->exhausted();
}
