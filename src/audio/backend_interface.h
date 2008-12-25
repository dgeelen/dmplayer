#ifndef BACKEND_INTERFACE_H
#define BACKEND_INTERFACE_H

#include "audio_controller.h"

class AudioController;

typedef boost::shared_ptr<class IBackend> IBackendRef;
class IBackend {
	private:
		AudioController* source;
	protected:
		AudioFormat af;
	public:
		IBackend(AudioController* i) : source(i) {};
		AudioFormat getAudioFormat() { return af;} ;
		virtual ~IBackend() {};
		virtual void start_output() = 0;
		virtual void stop_output() = 0;
};

#endif//BACKEND_INTERFACE_H
