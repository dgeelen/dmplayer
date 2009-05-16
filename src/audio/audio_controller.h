#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H
#include "decoder_interface.h"
#include "backend_interface.h"
#include "datasource_interface.h"
#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/signal.hpp>

class IBackend;
typedef boost::shared_ptr<IBackend> IBackendRef;

class AudioController {
	public:
		AudioController();
		~AudioController();
		void test_functie(std::string file);
		uint32 getData(uint8* buf, uint32 len);

		void start_playback();
		void stop_playback();
		void set_data_source(IDataSourceRef ds);
		boost::signal<void(uint64)> playback_finished;
	private:
		IBackendRef backend;
		IAudioSourceRef curdecoder;

		uint64 bytes_played;

		// possibly overkill here
		volatile bool update_decoder_flag;
		IAudioSourceRef update_decoder_source;
		boost::mutex update_decoder_mutex;
};

#endif//AUDIO_CONTROLLER_H
