#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H
#include "decoder_interface.h"
#include "backend_interface.h"
#include "datasource_interface.h"
#include <string>

class AudioController : public IDecoder {
	public:
		AudioController();
		~AudioController();
		IDecoder* tryDecode(IDataSource*);
		void test_functie(std::string file);
	private:
		IBackend* backend;
};

#endif//AUDIO_CONTROLLER_H
