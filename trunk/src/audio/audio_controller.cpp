#include <cstdlib>
#include <iostream>
#include "../cross-platform.h" // fixes header order problems

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
#include "filter_reformat.h"

using namespace std;

AudioController::AudioController()
	: update_decoder_flag(false)
{
	backend = NULL;
	#ifdef PORTAUDIO_BACKEND
	try {
		PortAudioBackend* be = new PortAudioBackend(this);
		backend = be;
		dcerr("AudioController: PortAudioBackend is available");
	} catch(...) {}
	#endif
	#if 0 //SDL_MIXER_BACKEND
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
		throw SoundException("AudioController(): Could not find a suitable backend!");
	}

	/* Now we have a backend */
}

AudioController::~AudioController() {
	delete backend;
}

uint32 AudioController::getData(uint8* buf, uint32 len)
{
	uint32 read = 0;
	if (update_decoder_flag) {
		// this might look like the double-checked locking anti-design-pattern, but...
		// it should work because of the mutexes
		boost::mutex::scoped_lock lock(update_decoder_mutex);
		curdecoder = update_decoder_source;
		update_decoder_source.reset();
		update_decoder_flag = false;
	}

	if (curdecoder) {
		read = curdecoder->getData(buf, len);
		if (read == 0)
			curdecoder.reset();
	}
	if (read < len) {
		memset(buf+read, 0, len-read);
		read = len;
	}
	return read;
}

void AudioController::set_data_source(IDataSourceRef ds) {
	IAudioSourceRef newdecoder = IDecoder::findDecoder(ds);
	if (!newdecoder) {
		dcerr("Cannot find decoder!");
		return;
	}

	if(newdecoder->getAudioFormat() != backend->getAudioFormat())
		newdecoder = IAudioSourceRef(new ReformatFilter(newdecoder, backend->getAudioFormat()));
	if(!newdecoder) dcerr("All decoders failed!");

	{
		boost::mutex::scoped_lock lock(update_decoder_mutex);
		update_decoder_flag = true;
		update_decoder_source = newdecoder;
		newdecoder.reset();
	}
}

void AudioController::test_functie(std::string file) {
	IDataSourceRef ds;
	if (!ds) {
		try {
			ds = IDataSourceRef(new FileReaderDataSource(file));
		}
		catch (Exception& e) {
			VAR_UNUSED(e); // in debug mode
			dcerr("Error message: " << e.what());
			ds.reset();
		}
	}

	if (!ds) {
		try {
			ds = IDataSourceRef(new HTTPStreamDataSource(file));
		}
		catch (Exception& e) {
			VAR_UNUSED(e); // in debug mode
			dcerr("Error message: " << e.what());
			ds.reset();
		}
	}

	if (!ds) {
		dcerr("Error opening: " << file);
		return;
	}

	set_data_source(ds);
}

void AudioController::StopPlayback()
{
	backend->StopStream();
}

void AudioController::StartPlayback()
{
	backend->StartStream();
}
