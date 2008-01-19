#ifndef BACKEND_LIBAO_H
#define BACKEND_LIBAO_H
#include <ao.h>
#include "backend_interface.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/barrier.hpp>

class LibAOBackend : public IBackend {
	public:
		LibAOBackend(IDecoder* dec);
		virtual ~LibAOBackend();
	private:
		IDecoder* decoder;

		ao_device *device;
		ao_sample_format format;
		int   playing_buffer;
		char* audio_buffer[2];
		int   audio_buffer_fill[2];
		bool  play_back;

		boost::barrier* fill_buffer_barrier;
		boost::mutex buffer_a_mutex;
		boost::mutex buffer_b_mutex;
		boost::thread* thread_decoder_read_thread;
		boost::thread* thread_ao_play_thread;
		void decoder_read_thread();
		void ao_play_thread();
};

#endif//BACKEND_LIBAO_H
