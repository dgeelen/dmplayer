#ifndef BACKEND_LIBAO_H
#define BACKEND_LIBAO_H
#include <ao.h>
#include "backend_interface.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/barrier.hpp>
#include <vector>

class LibAOBackend : public IBackend {
	public:
		LibAOBackend(AudioController* dec);
		virtual ~LibAOBackend();
	private:
		AudioController* decoder;

		ao_device *device;
		ao_sample_format format;
		int   playing_buffer;
		std::vector<std::vector<uint8> > audio_buffer;
		volatile bool read_back;
		volatile bool play_back;

		boost::barrier fill_buffer_barrier;
		boost::mutex   playback_lock_mutex;
		boost::shared_ptr<boost::thread> thread_decoder_read_thread;
		boost::shared_ptr<boost::thread> thread_ao_play_thread;
		void decoder_read_thread();
		void ao_play_thread();

		void start_output();
		void stop_output();
};

#endif//BACKEND_LIBAO_H
