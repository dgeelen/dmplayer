#ifndef BACKEND_PORTAUDIO_H
#define BACKEND_PORTAUDIO_H

#include "backend_interface.h"
#include "audio_controller.h"
#include "portaudio.h" // NOTE: *MUST* be portaudio V19

#include <boost/thread.hpp>

class PortAudioBackend : public IBackend {
	private:
		static int pa_callback(const void *inputBuffer, void *outputBuffer,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData );

		int data_callback(uint8* out, unsigned long byteCount);

		PaStream *stream;
		AudioController* dec;

		boost::mutex cb_mutex;
		std::vector<uint8> cbuf;
		size_t cb_rpos;
		size_t cb_wpos;
		size_t cb_data;
		size_t cb_size;

		void decodeloop();

		boost::thread decodethread;
	public:
		PortAudioBackend(AudioController* _dec);
		virtual ~PortAudioBackend();
		void start_output();
		void stop_output();
};

#endif//BACKEND_PORTAUDIO_H
