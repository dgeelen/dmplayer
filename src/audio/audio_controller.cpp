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
#include "datasource_httpstream.h"

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

uint32 AudioController::doDecode(uint8* buf, uint32 max, uint32 req)
{
	uint32 read = 0;
	if (curdecoder)
		read = curdecoder->doDecode(buf, max, req);
	if (read < req) {
		memset(buf+read, 0, req-read);
		read = req;
	}
	return read;
}

void AudioController::test_functie(std::string file) {
	IDataSource* ds = NULL;
	if (ds == NULL) {
		try {
			ds = new FileReaderDataSource(file);
		}
		catch (std::string error_msg) {
			dcerr("Error message: "<<error_msg);
			ds = NULL;
		}
		catch (char* error_msg) {
			dcerr("Error message: "<<error_msg);
			ds = NULL;
		}
	}

	if (ds == NULL) {
		try {
			ds = new HTTPStreamDataSource(file);
		}
		catch (std::string error_msg) {
			dcerr("Error message: "<<error_msg);
			ds = NULL;
		}
		catch (char* error_msg) {
			dcerr("Error message: "<<error_msg);
			ds = NULL;
		}
	}

	if (ds == NULL)
		dcerr("Error opening :" << file);
	else
	{
		for (unsigned int i = 0; i < decoderlist.size(); ++i) {
			IDecoder* decoder = decoderlist[i](ds);
			if (decoder) {
				curdecoder = decoder;
				dcerr("Found a decoder; decoder #"<<i);
				return;
			}
		}
	}
}
