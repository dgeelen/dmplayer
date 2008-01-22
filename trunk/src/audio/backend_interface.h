#ifndef BACKEND_INTERFACE_H
#define BACKEND_INTERFACE_H

#include "audio_controller.h"

class IBackend {
	private:
		AudioController* source;
	protected:
		AudioFormat af;
	public:
		IBackend(AudioController* i) : source(i) {};
		AudioFormat getAudioFormat() { return af;} ;
		virtual ~IBackend() {};
};

#endif//BACKEND_INTERFACE_H
