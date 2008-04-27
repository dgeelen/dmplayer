#ifndef BACKEND_PORTAUDIO_H
#define BACKEND_PORTAUDIO_H

#include "backend_interface.h"

#include "portaudio.h"

// ugh, how ugly is this...
#ifdef PORT_AUDIO_H
#define PA_VERSION 18
#endif
#ifdef PORTAUDIO_H
#define PA_VERSION 19
#endif

class PortAudioBackend : public IBackend{
	private:
#if PA_VERSION == 18
		PortAudioStream *stream;
#endif
#if PA_VERSION == 19
		PaStream *stream;
#endif
	public:
		PortAudioBackend(AudioController* dec);
		virtual ~PortAudioBackend();
		void StartStream();
		void StopStream();
};

#endif//BACKEND_PORTAUDIO_H
