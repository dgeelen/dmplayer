#ifndef AUDIOSOURCE_INTERFACE_H
#define AUDIOSOURCE_INTERFACE_H

#include "../types.h"
#include <iostream>
#include <boost/shared_ptr.hpp>

struct AudioFormat {
	uint32 SampleRate;
	uint32 Channels;
	uint32 BitsPerSample;
	bool SignedSample;
	bool LittleEndian;
	bool Float;

	uint32 getBytesPerSecond() {
		return (SampleRate * Channels * BitsPerSample) / 8;
	}

	AudioFormat() {
		SampleRate = uint32(-1);
		Channels = 2;
		BitsPerSample = 16;
		SignedSample = true;
		LittleEndian = true;
		Float = false;
	}

	bool operator==(const AudioFormat& af) const {
		if (SampleRate    != af.SampleRate   ) return false;
		if (Channels      != af.Channels     ) return false;
		if (BitsPerSample != af.BitsPerSample) return false;
		if (SignedSample  != af.SignedSample ) return false;
		if (LittleEndian  != af.LittleEndian ) return false;
		if (Float         != af.Float        ) return false;
		return true;
	}

	bool operator!=(const AudioFormat& af) const  {
		return !(*this == af);
	}
};

class IAudioSource {
	protected:
		AudioFormat audioformat;

	public:
		IAudioSource(AudioFormat af) { audioformat = af;};
		virtual ~IAudioSource() {};

		const AudioFormat getAudioFormat() { return audioformat; };
		virtual bool exhausted() = 0;
		virtual uint32 getData(uint8* buf, uint32 max) = 0;
};
typedef boost::shared_ptr<IAudioSource> IAudioSourceRef;

#endif//AUDIOSOURCE_INTERFACE_H
