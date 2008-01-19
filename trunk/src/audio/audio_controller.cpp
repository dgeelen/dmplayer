#include <cstdlib>
#include <iostream>
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

/* FOR TESTING PURPOSES ONLY */
#include "datasource_filereader.h"

using namespace std;

AudioController::AudioController() : IDecoder() {
	curdecoder = NULL;
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
	if (!curdecoder) return 0;
	return curdecoder->doDecode(buf, max, req);
}

void AudioController::test_functie(std::string file) {
	try {
	FileReaderDataSource* ds = new FileReaderDataSource(file);
	//*
	FileReaderDataSource* FRDS = new FileReaderDataSource(file);
	for (unsigned int i = 0; i < decoderlist.size(); ++i) {
		IDecoder* decoder = decoderlist[i](FRDS);
		if (decoder) {
			curdecoder = decoder;
			return;
		}
	}
	/*/
	OGGDecoder* decoder = new OGGDecoder();
	curdecoder = decoder;
	decoder->tryDecode(ds);
	/**/
	}
	catch(...) {
		cout << "Error while trying to play '"<<file<<"'\n";
	}
}
