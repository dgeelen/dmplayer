#include "filter_upmix.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include "filter_sampleconverter.h"
#include "filter_split.h"
#include <algorithm>
#include <limits>

using namespace std;

/**
 * UpmixFilter accepts any input format (it detects non-float and adds the
 * SampleConverterFilter on it's own. UpmixFilter again outputs Float.
 */
UpmixFilter::UpmixFilter(IAudioSourceRef as, AudioFormat target)
: IAudioSource(as->getAudioFormat()), src(as), target(target)
{
	if((src->getAudioFormat().Channels != 2) && (target.Channels != 6)) {
		throw SoundException(STRFORMAT("Unsupported upmix: %i -> %i", src->getAudioFormat().Channels, target.Channels));
	}

	if(!src->getAudioFormat().Float) { // both IIR filter *and* resample filter expect Float input
		AudioFormat af_float(src->getAudioFormat());
		af_float.Float = true;
		af_float.BitsPerSample = 32;
		audioformat = af_float;
		dcerr("Auto inserting SampleConverterFilter");
		src = IAudioSourceRef(new SampleConverterFilter(src, af_float));
	}
	audioformat.Channels = target.Channels;
}

/*
UpmixFilter::~UpmixFilter() {
}
*/

uint32 UpmixFilter::getData(uint8* buf, uint32 len) {
	#define INPUT_SIZE 8196 
	float input[INPUT_SIZE];
	uint bytes_per_src_sample = src->getAudioFormat().Channels    * (src->getAudioFormat().BitsPerSample>>3);
	uint bytes_per_tgt_sample = target.Channels * 4; // We output float, so 4 bytes per channel
	uint n_samples_req        = len / bytes_per_tgt_sample;
	uint n_samples_fit        = INPUT_SIZE / bytes_per_src_sample;
	uint samples_todo         = std::min(n_samples_req, n_samples_fit);
	uint bytes_needed         = samples_todo * bytes_per_src_sample;
	uint bytes_got            = src->getData((uint8*)input, bytes_needed);
	samples_todo              = bytes_got / bytes_per_src_sample;
	if(false && (src->getAudioFormat().Channels == 2) && (target.Channels == 6)) { // Dedicated surround upmixer
		float* s = input;
		float* d = (float*)buf;
		for(uint sample = 0 ; sample < samples_todo; ++sample) {
			float f = (*(s++) + *(s++)) / 2.0f;
			*(d++) = (1.0f / 6.0f) * 0.5f * (((int)sample&1)*2 - 1); // Front Left
			*(d++) = (1.0f / 6.0f) * 1.5f * (((int)sample&1)*2 - 1); // Front Right
			*(d++) = (1.0f / 6.0f) * 2.5f * (((int)sample&1)*2 - 1); // Rear Left
			*(d++) = (1.0f / 6.0f) * 3.5f * (((int)sample&1)*2 - 1); // Rear Right
			*(d++) = (1.0f / 6.0f) * 4.5f * (((int)sample&1)*2 - 1); // Center
			*(d++) = (1.0f / 6.0f) * 5.5f * (((int)sample&1)*2 - 1); // Sub Woofer
			//for(uint src_chan = 0; src_chan < src->getAudioFormat().Channels; src_chan++) {
			//}
		}
	}
	else { // Generic upmixer, repeats input channels on output channels alternatingly
		float* s = input;
		float* d = (float*)buf;
		for(uint sample = 0 ; sample < samples_todo; ++sample) {
			for(uint src_chan = 0; src_chan < src->getAudioFormat().Channels; src_chan++) {
				for(uint dst_chan = src_chan; dst_chan < target.Channels; dst_chan += src->getAudioFormat().Channels) {
					*(d+dst_chan) = *s; // src_chan / double(src->getAudioFormat().Channels) * (((int)sample&1)*2 - 1);
				}
				++s;
			}
			d += target.Channels;
		}
	}
	return samples_todo * bytes_per_tgt_sample;
}

bool UpmixFilter::exhausted() {
	return src->exhausted();
}
