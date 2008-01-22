#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H
#include "decoder_interface.h"
#include "datasource_interface.h"
#include <string>

class IBackend;

class AudioController {
	public:
		AudioController();
		~AudioController();
		IDecoder* tryDecode(IDataSource*);
		void test_functie(std::string file);
		uint32 getData(uint8* buf, uint32 len);
	private:
		IBackend* backend;
		IDecoder* curdecoder;
};

#endif//AUDIO_CONTROLLER_H
