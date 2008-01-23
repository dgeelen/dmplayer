#include "filter_reformat.h"

#include "../cross-platform.h"
#include <algorithm>

#include "filter_sampledoubler.h"

using namespace std;

ReformatFilter::ReformatFilter(IAudioSource* as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	// partially fix sample rate (only multiply by powers of 2)
	while (src->getAudioFormat().SampleRate < target.SampleRate)
		src = new SampleDoublerFilter(src);

	audioformat = src->getAudioFormat();
}

uint32 ReformatFilter::getData(uint8* buf, uint32 len)
{
	return src->getData(buf, len);
}
