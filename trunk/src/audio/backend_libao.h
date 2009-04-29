#ifndef BACKEND_LIBAO_H
#define BACKEND_LIBAO_H
#include "backend_interface.h"
#include <ao.h>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/condition.hpp>

class LibAOBackend : public IBackend {
	public:
		LibAOBackend(AudioController* dec);
		virtual ~LibAOBackend();
	private:
		void decoder_read_thread();
		void ao_play_thread();
		void start_output();
		void stop_output();

		AudioController* decoder;
		ao_device *device;
		ao_sample_format format;
		std::vector<std::vector<uint8> > audio_buffer;

		bool active;
		boost::barrier   thread_started_barrier;
		boost::mutex     readback_lock_mutex;
		boost::condition read_next_condition;
		boost::mutex     playback_lock_mutex;
		boost::condition play_next_condition;

		boost::shared_ptr<boost::thread> thread_decoder_read_thread;
		boost::shared_ptr<boost::thread> thread_ao_play_thread;
};

#endif//BACKEND_LIBAO_H
