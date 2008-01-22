#ifndef AUDIOSOURCE_INTERFACE_H
#define AUDIOSOURCE_INTERFACE_H

#include "../types.h"

struct AudioFormat{
	int SampleRate;
	int Channels;
	int BitsPerSample;
	bool SignedSample;
	bool LittleEndian;
};

class IAudioSource {
	private:
		AudioFormat audioformat;

	public:
		IAudioSource(AudioFormat af) { audioformat = af;};
		virtual ~IAudioSource() {};

		const AudioFormat getAudioFormat() {return audioformat;};

		virtual uint32 getData(uint8* buf, uint32 max) = 0;
};

#endif//AUDIOSOURCE_INTERFACE_H
