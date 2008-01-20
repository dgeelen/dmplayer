#ifndef BACKEND_PORTAUDIO_H
#define BACKEND_PORTAUDIO_H

#include "backend_interface.h"

#include "portaudio.h"

class PortAudioBackend : public IBackend{
	private:
		PortAudioStream *stream;
	public:
		PortAudioBackend(IDecoder* dec);
		virtual ~PortAudioBackend();
};

#endif//BACKEND_PORTAUDIO_H