#ifndef BACKEND_PORTAUDIO_H
#define BACKEND_PORTAUDIO_H

#include "backend_interface.h"
#include "audio_controller.h"
#include "portaudio.h" // NOTE: *MUST* be portaudio V19

class PortAudioBackend : public IBackend {
	private:
		static int pa_callback(const void *inputBuffer, void *outputBuffer,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData );
		PaStream *stream;
		AudioController* dec;

	public:
		PortAudioBackend(AudioController* _dec);
		virtual ~PortAudioBackend();
		void start_output();
		void stop_output();
};

#endif//BACKEND_PORTAUDIO_H
