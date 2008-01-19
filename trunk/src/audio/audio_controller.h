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
		uint32 doDecode(char* buf, uint32 max, uint32 req);
	private:
		IBackend* backend;
};

#endif//AUDIO_CONTROLLER_H
