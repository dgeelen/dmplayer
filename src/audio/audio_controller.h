#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H
#include "decoder_interface.h"
#include "datasource_interface.h"
#include <string>
#include <boost/thread/mutex.hpp>

class IBackend;

class AudioController {
	public:
		AudioController();
		~AudioController();
		void test_functie(std::string file);
		uint32 getData(uint8* buf, uint32 len);
		void StartPlayback();
		void StopPlayback();
		void set_data_source(IDataSourceRef ds);
	private:
		IBackend* backend;
		IAudioSourceRef curdecoder;

		// possibly overkill here
		volatile bool update_decoder_flag;
		IAudioSourceRef update_decoder_source;
		boost::mutex update_decoder_mutex;
};

#endif//AUDIO_CONTROLLER_H
