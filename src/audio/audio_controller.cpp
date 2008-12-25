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
#ifdef LIBAO_BACKEND
	#include "backend_libao.h"
#endif

/* FOR TESTING PURPOSES ONLY */
#include "datasource_filereader.h"
#include "datasource_httpstream.h"
#include "filter_reformat.h"
#include "backend_wavwriter.h"

// TODO, also poll for data...
class NullBackend: public IBackend {
	public:
		NullBackend(AudioController* i) : IBackend(i) {};
		AudioFormat getAudioFormat() { return af;} ;
		virtual ~NullBackend() {};
		virtual void start_output() {};
		virtual void stop_output()  {};
};

using namespace std;

AudioController::AudioController()
	: update_decoder_flag(false)
{
	bytes_played = 0;
	backend.reset();
	#ifdef PORTAUDIO_BACKEND
	try {
		backend = IBackendRef(new PortAudioBackend(this));
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
		backend = IBackendRef(new LibAOBackend(this));
		dcerr("AudioController: LibAOBackend is available");
	} catch(...) {}
	#endif
	#if 0
	try {
		WAVWriterBackend* be = new WAVWriterBackend(this);
		backend = be;
		dcerr("AudioController: WAVWriterBackend is available");
	} catch(...) {}
	#endif
	if(backend == NULL) {
		backend = IBackendRef(new NullBackend(this));
		//throw SoundException("AudioController(): Could not find a suitable backend!");
		dcerr("AudioController(): Could not find a suitable backend!");
	}

	/* Now we have a backend */
}

AudioController::~AudioController() {
	dcerr("Shutting down");
	stop_playback();
	backend.reset();
	dcerr("Shut down");
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
		bytes_played += read;
	}
	if (read < len) { //FIXME: Really only if curdecoder->exhausted()
		memset(buf+read, 0, len-read);
		read = len; // Disable this when using the WAVWriterBackend
		if(curdecoder && curdecoder->exhausted()) {
			playback_finished(bytes_played / backend->getAudioFormat().getBytesPerSecond()); //FIXME: Low resolution!
			bytes_played = 0;
			curdecoder.reset();
		}
	}
	return read;
}

void AudioController::set_data_source(const IDataSourceRef ds) {
	IAudioSourceRef newdecoder = IDecoder::findDecoder(ds);
	if (!newdecoder) {
		try {
			ds->reset();
			vector<uint8> svec;
			svec.resize(4096);
			int read = ds->getData(&svec[0], 4096);
			string s;
			s.resize(read);
			for(int i = 0; i<read; ++i) {
				s[i] = svec[i];
			}
			IDataSourceRef httpdatasource = IDataSourceRef(new HTTPStreamDataSource(s));
			newdecoder = IDecoder::findDecoder(httpdatasource);
		}
		catch (Exception& e) {
			VAR_UNUSED(e); // in debug mode
			dcerr("Error message: " << e.what());
			ds->reset();
		}
		if(!newdecoder) {
			dcerr("Cannot find decoder!");
			playback_finished(0); //FIXME: Low resolution!
			return;
		}
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

void AudioController::stop_playback()
{
	dcerr("Stopping backend");
	backend->stop_output();
}

void AudioController::start_playback()
{
	dcerr("Starting backend");
	backend->start_output();
}
