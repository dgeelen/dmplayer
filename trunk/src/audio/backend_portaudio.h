#ifndef BACKEND_PORTAUDIO_H
#define BACKEND_PORTAUDIO_H

#include "backend_interface.h"

#include <portaudio.h>

#define TABLE_SIZE   (200)
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
}
paTestData;

class PortAudioBackend : public IBackend{
	private:
		PortAudioStream *stream;
		paTestData data;
	public:
		PortAudioBackend(IDecoder* dec);
		virtual ~PortAudioBackend() {};
};

#endif//BACKEND_PORTAUDIO_H
