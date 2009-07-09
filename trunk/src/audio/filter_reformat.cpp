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
		throw Exception("Invalid audioformat! (source or target samplerate unset while matching audioformats)");
	}

	uint32 nfilters = 0;
	while(nfilters < 32 && src->getAudioFormat() != target) {
		// mono to stereo if needed
		if (src->getAudioFormat().Channels == 1 && target.Channels == 2) {
			if(src->getAudioFormat().Float
			|| src->getAudioFormat().BitsPerSample != 16
			) {
				AudioFormat s16_target(src->getAudioFormat());
				s16_target.Float = false;
				s16_target.BitsPerSample = 16;
				src = IAudioSourceRef(new SampleConverterFilter(src, s16_target));
			}
			dcerr(src->getAudioFormat().Channels << " != " << target.Channels);
			src = IAudioSourceRef(new MonoToStereoFilter(src));
			++nfilters;
		}

		if(src->getAudioFormat().SampleRate != target.SampleRate) {
		#ifdef LIBSAMPLERATE_FILTER
			dcerr("Using libsamplerate filter: " << src->getAudioFormat().SampleRate << "->" << target.SampleRate);
			if(!src->getAudioFormat().Float) {
				AudioFormat float_target(target);
				float_target.Float = true;
				src = IAudioSourceRef(new SampleConverterFilter(src, float_target));
			}
			src = IAudioSourceRef(new LibSamplerateFilter(src, target));
		#else
			// partially fix sample rate (only multiply by powers of 2)
			int count = 0;
			while ((src->getAudioFormat().SampleRate < target.SampleRate) && (count++<3)) {
				dcerr(src->getAudioFormat().SampleRate << " < " << target.SampleRate);
				src = IAudioSourceRef(new SampleDoublerFilter(src));
			}
			dcerr(src->getAudioFormat().SampleRate << " < " << target.SampleRate);
		#endif
			++nfilters;
		}

		if((src->getAudioFormat().Float         != target.Float)
		|| (src->getAudioFormat().SignedSample  != target.SignedSample)
		|| (src->getAudioFormat().LittleEndian  != target.LittleEndian)
		|| (src->getAudioFormat().BitsPerSample != target.BitsPerSample)
		) {
			src = IAudioSourceRef(new SampleConverterFilter(src, target));
			++nfilters;
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
