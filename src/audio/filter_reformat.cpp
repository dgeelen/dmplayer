#include "filter_reformat.h"

#include "../cross-platform.h"
#include "../error-handling.h"
#include <algorithm>

#include "filter_sampledoubler.h"
#include "filter_monotostereo.h"
#include "filter_sampleconverter.h"
#ifdef LIBSAMPLERATE_FILTER
	#include "filter_libsamplerate.h"
#endif

using namespace std;

ReformatFilter::ReformatFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	dcerr("Trying to match audioformats");
	// Abort on invalid AudioFormat
	if(src->getAudioFormat().Channels==-1 || target.Channels==-1 ||
	   src->getAudioFormat().SampleRate==-1 || target.SampleRate==-1 ) {
		throw Exception("Invalid audioformat!");
	}

	uint32 nfilters = 0;
	while(nfilters < 32 && src->getAudioFormat() != target) {
		// mono to stereo if needed
		if (src->getAudioFormat().Channels == 1 && target.Channels == 2) {
			dcerr(src->getAudioFormat().Channels << " != " << target.Channels);
			src = IAudioSourceRef(new MonoToStereoFilter(src));
		}

		if(src->getAudioFormat().SampleRate != target.SampleRate) {
		#ifdef LIBSAMPLERATE_FILTER
			dcerr("Using libsamplerate filter: " << src->getAudioFormat().SampleRate << "->" << target.SampleRate);
			AudioFormat float_target(target);
			float_target.Float = true;
			IAudioSourceRef to_float_format(new SampleConverterFilter(src,    float_target));
			IAudioSourceRef resample(new LibSamplerateFilter(to_float_format, float_target));
			IAudioSourceRef to_target_format(new SampleConverterFilter(resample,    target));
			src = to_target_format;
		#else
			// partially fix sample rate (only multiply by powers of 2)
			int count = 0;
			while ((src->getAudioFormat().SampleRate < target.SampleRate) && (count++<3)) {
				dcerr(src->getAudioFormat().SampleRate << " < " << target.SampleRate);
				src = IAudioSourceRef(new SampleDoublerFilter(src));
			}
			dcerr(src->getAudioFormat().SampleRate << " < " << target.SampleRate);
		#endif
		}
	}
	audioformat = src->getAudioFormat();
}

bool ReformatFilter::exhausted() {
	return src->exhausted();
}

uint32 ReformatFilter::getData(uint8* buf, uint32 len)
{
	return src->getData(buf, len);
}
