#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H
#include "decoder_interface.h"
#include "backend_interface.h"
#include "datasource_interface.h"

class AudioController : public IDecoder {
	public:
		AudioController();
		~AudioController();
		IDecoder* tryDecode(IDataSource*);
	private:
		IBackend* backend;
};

#endif//AUDIO_CONTROLLER_H
