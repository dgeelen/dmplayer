#include "filter_reformat.h"

#include "../cross-platform.h"
#include <algorithm>

#include "filter_sampledoubler.h"
#include "filter_monotostereo.h"

using namespace std;

ReformatFilter::ReformatFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	// mono to stereo if needed
	if (src->getAudioFormat().Channels == 1 && target.Channels == 2)
		src = IAudioSourceRef(new MonoToStereoFilter(src));

	// partially fix sample rate (only multiply by powers of 2)
	while (src->getAudioFormat().SampleRate < target.SampleRate)
		src = IAudioSourceRef(new SampleDoublerFilter(src));


	audioformat = src->getAudioFormat();
}

uint32 ReformatFilter::getData(uint8* buf, uint32 len)
{
	return src->getData(buf, len);
}
