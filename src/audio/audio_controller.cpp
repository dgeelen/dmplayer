#include <cstdlib>
#include "audio_controller.h"
#include "backend_interface.h"
#include "../error-handling.h"
#ifdef PORTAUDIO_BACKEND
	#include "backend_portaudio.h"
#endif
#ifdef SDL_MIXER_BACKEND
	#include "backend_sdlmixer.h"
#endif
#ifdef SDL_MIXER_BACKEND
	#include "backend_libao.h"
#endif
#ifdef LIBOGG_DECODER
	#include "decoder_libogg.h"
#endif

/* FOR TESTING PURPOSES ONLY */
#include "datasource_filereader.h"
#include "decoder_mad.h"

AudioController::AudioController() : IDecoder() {
	#ifdef PORTAUDIO_BACKEND
	backend = NULL;
	try {
		PortAudioBackend* be = new PortAudioBackend(this);
		backend = be;
		dcerr("AudioController: PortAudioBackend is available");
	} catch(...) {}
	#endif
	#ifdef SDL_MIXER_BACKEND
	try {
		SDLMixerBackend* be = new SDLMixerBackend(this);
		backend = be;
		dcerr("AudioController: SDLMixerBackend is available");
	} catch(...) {}
	#endif
	#ifdef LIBAO_BACKEND
	try {
		LibAOBackend* be = new LibAOBackend(this);
		backend = be;
		dcerr("AudioController: LibAOBackend is available");
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

uint32 AudioController::doDecode(char* buf, uint32 max, uint32 req)
{
	return 0;
}

void AudioController::test_functie(std::string file) {
/*
	std::cerr << "blaat1 \n";
	MadDecoder* DE = new MadDecoder();
	std::cerr << "blaat2 \n";
	FileReaderDataSource* FRDS = new FileReaderDataSource(file);
	std::cerr << "blaat3 \n";
	PortAudioBackend* BE = new PortAudioBackend(DE);
	std::cerr << "blaat4 \n";
*/
}
