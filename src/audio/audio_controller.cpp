#include <cstdlib>
#include "audio_controller.h"
#include "backend_interface.h"
#include "../error-handling.h"
#ifdef PORT_AUDIO_BACKEND
	#include "backend_portaudio.h"
#endif
#ifdef SDL_MIXER_BACKEND
	#include "backend_sdlmixer.h"
#endif

AudioController::AudioController() : IDecoder() {
	#ifdef PORTAUDIO_BACKEND
	backend = NULL;
	try {
		PortAudioBackend* be = new PortAudioBackend(this);
		backend = be;
		dcerr("AudioController: using PortAudioBackend");
	} catch(...) {}
	#endif
	#ifdef SDL_MIXER_BACKEND
	try {
		SDLMixerBackend* be = new SDLMixerBackend(this);
		backend = be;
		dcerr("AudioController: using SDLMixerBackend");
	} catch(...) {}
	#endif
	if(backend == NULL) {
		throw "AudioController(): Could not find a suitable backend!";
	}

	/* Now we have a backend */

}

AudioController::~AudioController() {
	delete backend;
}

IDecoder* AudioController::tryDecode(IDataSource* ds) {
	//TODO:
	return this;
}
