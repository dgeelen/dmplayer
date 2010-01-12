#include "filter_downmix.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include "filter_sampleconverter.h"

using namespace std;

/**
 * DownmixFilter accepts any input format (it detects non-float and adds the
 * SampleConverterFilter on it's own. DownmixFilter outputs Float.
 */
DownmixFilter::DownmixFilter(IAudioSourceRef as, AudioFormat target)
: IAudioSource(as->getAudioFormat()), src(as), target(target)
{
	if((src->getAudioFormat().Channels < 2) && (target.Channels != 1)) {
		throw SoundException(STRFORMAT("Unsupported downmix: %i -> %i", src->getAudioFormat().Channels, target.Channels));
	}

	if(!src->getAudioFormat().Float) {
		AudioFormat af_float(src->getAudioFormat());
		af_float.Float = true;
		af_float.BitsPerSample = 32;
		audioformat = af_float;
		dcerr("Auto inserting SampleConverterFilter");
		src = IAudioSourceRef(new SampleConverterFilter(src, af_float));
	}
	audioformat.Channels = target.Channels;
}

uint32 DownmixFilter::getData(uint8* buf, uint32 len) {
	#define INPUT_SIZE 8196 
	float input[INPUT_SIZE];
	uint bytes_per_src_sample = src->getAudioFormat().Channels * (src->getAudioFormat().BitsPerSample>>3);
	uint bytes_per_tgt_sample = target.Channels * 4; // We output float, so 4 bytes per channel
	uint n_samples_req        = len / bytes_per_tgt_sample;
	uint n_samples_fit        = INPUT_SIZE / bytes_per_src_sample;
	uint samples_todo         = std::min(n_samples_req, n_samples_fit);
	uint bytes_needed         = samples_todo * bytes_per_src_sample;
	uint bytes_got            = src->getData((uint8*)input, bytes_needed);
	samples_todo              = bytes_got / bytes_per_src_sample;
	if((src->getAudioFormat().Channels == 2) && (target.Channels == 1)) { // Stereo to mono
		float* s = input;
		float* d = (float*)buf;
		for(uint sample = 0 ; sample < samples_todo; ++sample) {
			*(d++) = (s[0] + s[1]) * 0.5;
			s+=2;
		}
	}
	else { // TODO: Generic downmixer or so?
		throw SoundException(STRFORMAT("DownmixFilter: cannot downmix %d channels into %d channels", src->getAudioFormat().Channels, target.Channels));
	}
	return samples_todo * bytes_per_tgt_sample;
}

bool DownmixFilter::exhausted() {
	return src->exhausted();
}
