#include <cstdlib>
#include <iostream>
#include "../cross-platform.h" // fixes header order problems

#include "audio_controller.h"
#include "backend_interface.h"
#include "../error-handling.h"
#ifdef PORTAUDIO_BACKEND
	#include "backend_portaudio.h"
#endif

/* FOR TESTING PURPOSES ONLY */
#include "datasource_filereader.h"
#include "datasource_httpstream.h"
#include "filter_reformat.h"
#include "backend_wavwriter.h"
#include "filter_normalize.h"
#include "filter_downmix.h"

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

AudioController::AudioController(int preferred_output_channel_count_)
	: preferred_output_channel_count(preferred_output_channel_count_)
	, update_decoder_flag(false)
	, started(false)
{
	initialize();
}

AudioController::AudioController()
	: preferred_output_channel_count(6)
	, update_decoder_flag(false)
	, started(false)
{
	initialize();
}

void AudioController::initialize() {
	bytes_played = 0;
	backend.reset();
	dcerr("preferred_output_channel_count:" << preferred_output_channel_count);
	#ifdef PORTAUDIO_BACKEND
	try {
		backend = IBackendRef(new PortAudioBackend(this));
		dcerr("AudioController: PortAudioBackend is available");
	} catch(...) {}
	#endif
	#if 0
	try {
		backend = IBackendRef(new WAVWriterBackend(this));
		dcerr("AudioController: WAVWriterBackend is available");
	} catch(...) {}
	#endif
	if(backend == NULL) {
		backend = IBackendRef(new NullBackend(this));
		//throw SoundException("AudioController(): Could not find a suitable backend!");
		cerr << "AudioController: Error: Could not find a suitable backend!" << endl;
	}

	/* Now we have a backend */
	start_playback();
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
		if (!curdecoder) {
			playback_finished(get_current_playtime()); //FIXME: Low resolution!
			bytes_played = 0;
		}
	}

	if (curdecoder) {
		try {
			read = curdecoder->getData(buf, len);
			bytes_played += read;
		} catch (std::exception&) {
			playback_finished(get_current_playtime()); //FIXME: Low resolution!
			bytes_played = 0;
			curdecoder.reset();
		}
	}
	if (read < len && curdecoder && curdecoder->exhausted()) {
		playback_finished(get_current_playtime()); //FIXME: Low resolution!
		bytes_played = 0;
		curdecoder.reset();
	}
	return read;
}

uint64 AudioController::get_current_playtime() {
	return bytes_played / backend->getAudioFormat().getBytesPerSecond(); //FIXME: Low resolution!
}

void AudioController::set_data_source(const IDataSourceRef ds) {
	IAudioSourceRef newdecoder;
	if(ds) {
		newdecoder = IDecoder::findDecoder(ds);
		if (!newdecoder) {
			dcerr("Cannot find decoder!");
			playback_finished(0);
			return;
		}

		#ifndef DEBUG // Uses too much CPU to do any usefull debugging, so leave it disabled unless needed
		newdecoder = IAudioSourceRef(new NormalizeFilter(newdecoder, backend->getAudioFormat()));
		#endif

		if(newdecoder->getAudioFormat() != backend->getAudioFormat())
			newdecoder = IAudioSourceRef(new ReformatFilter(newdecoder, backend->getAudioFormat()));
	}

	if (newdecoder) {
		boost::mutex::scoped_lock lock(update_decoder_mutex);
		update_decoder_source = newdecoder;
		update_decoder_flag = true;
	}
}

void AudioController::test_functie(std::string file) {
	IDataSourceRef ds;
	if (!ds) {
		try {
			ds = IDataSourceRef(new FileReaderDataSource(file));
		}
		catch (std::exception& e) {
			VAR_UNUSED(e); // in debug mode
			dcerr("Error message: " << e.what());
			ds.reset();
		}
	}

	if (!ds) {
		try {
			ds = IDataSourceRef(new HTTPStreamDataSource(file));
		}
		catch (std::exception& e) {
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
	if (!started) return;
	dcerr("Stopping backend");
	backend->stop_output();
}

void AudioController::start_playback()
{
	if (started) return;
	dcerr("Starting backend");
	backend->start_output();
}

void AudioController::abort_current_decoder()
{
	boost::mutex::scoped_lock lock(update_decoder_mutex);
	update_decoder_source.reset();
	update_decoder_flag = true;
}
