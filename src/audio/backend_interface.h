#ifndef BACKEND_INTERFACE_H
#define BACKEND_INTERFACE_H

#include "audio_controller.h"

class IBackend {
	private:
		AudioController* source;
	public:
		IBackend(AudioController* i) : source(i) {};
		virtual ~IBackend() {};
};

#endif//BACKEND_INTERFACE_H
