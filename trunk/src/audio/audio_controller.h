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
		AudioController(int preferred_output_channel_count_);
		~AudioController();
		void test_functie(std::string file);
		int get_preferred_output_channel_count() { return preferred_output_channel_count; };
		uint32 getData(uint8* buf, uint32 len);

		void start_playback();
		void stop_playback();
		void abort_current_decoder();

		void set_data_source(IDataSourceRef ds);
		uint64 get_current_playtime();
		boost::signal<void(uint64)> playback_finished;

	private:
		void initialize();
		int preferred_output_channel_count;
		IBackendRef backend;
		IAudioSourceRef curdecoder;

		uint64 bytes_played;

		bool started;

		// possibly overkill here
		volatile bool update_decoder_flag;
		IAudioSourceRef update_decoder_source;
		boost::mutex update_decoder_mutex;
};

#endif//AUDIO_CONTROLLER_H
