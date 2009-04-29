#include "backend_libao.h"
#include "../error-handling.h"
#include <boost/bind.hpp>

#define BUF_SIZE 1024*8 /* 8K buffers =~ 0.0464399sec of buffer) */
LibAOBackend::LibAOBackend(AudioController* dec)	: IBackend(dec), thread_started_barrier(2) {
	if(!decoder) throw(Exception("Error: decoder is NULL!"));
	ao_initialize();
	int default_driver = ao_default_driver_id();
	format.bits = 16;
	format.channels = 2;
	format.rate = 44100;
	format.byte_format = AO_FMT_LITTLE;

	/* let the world know what audio format we accept */
	af.Channels = format.channels;
	af.BitsPerSample = format.bits;
	af.SampleRate = format.rate;
	af.SignedSample = true;
	af.LittleEndian = true;

	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		dcerr("LibAOBackend: Error opening device!");
		throw SoundException("LibAOBackend: Error opening device!");
	}

	/* Initialize playback buffers, we use double buffering */
	audio_buffer.resize(2);
	for(int i=0; i<2; ++i) {
		audio_buffer[i].resize(BUF_SIZE);
		for(int j=0; j<BUF_SIZE; ++j) {
			audio_buffer[i][j] = 0;
		}
	}
	decoder = dec;
	active=false;
}

void LibAOBackend::decoder_read_thread() {
	int buffer_id = 0;
	boost::mutex::scoped_lock lock(readback_lock_mutex);
	thread_started_barrier.wait(); // wait for ao_play_thread to start
	while(active) {
		uint32 todo = BUF_SIZE;
		while(active && todo>0)
				todo -= decoder->getData(&(audio_buffer[buffer_id])[0] + (BUF_SIZE - todo), todo);
		playback_lock_mutex.lock();       // can only lock when playback thread is waiting
		playback_lock_mutex.unlock();     // release
		play_next_condition.notify_one(); // and notify playback thread
		read_next_condition.wait(lock);   // unlocks readback_lock_mutex, wait until playback thread notifies us
		buffer_id = 1 - buffer_id;
	}
}

void LibAOBackend::ao_play_thread() {
	int buffer_id = 0;
	boost::mutex::scoped_lock lock(playback_lock_mutex);
	thread_started_barrier.wait(); // wait for decoder_read_thread to start
	while(active) {
		play_next_condition.wait(lock);   // unlocks playback_lock_mutex
		readback_lock_mutex.lock();       // can only lock when readback thread is waiting
		readback_lock_mutex.unlock();     // release
		read_next_condition.notify_one(); // and notify
		ao_play(device, (char*)&audio_buffer[buffer_id][0], BUF_SIZE);
		buffer_id = 1 - buffer_id;
	}
}

void LibAOBackend::start_output() {
	if(!active) {
		dcerr("starting...");
		active = true;
		try {
			thread_ao_play_thread      = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(boost::bind(&LibAOBackend::ao_play_thread, this))));
			thread_decoder_read_thread = boost::shared_ptr<boost::thread>(new boost::thread(makeErrorHandler(boost::bind(&LibAOBackend::decoder_read_thread, this))));
		}
		catch(...) {
			throw ThreadException("LibAOBackend: Could not start output thread!");
		}
	}
	dcerr("started");
}

void LibAOBackend::stop_output() {
	dcerr("stopping...");
	readback_lock_mutex.lock(); // Only when decoder_read_thread is waiting
	active = false;
	readback_lock_mutex.unlock();
	read_next_condition.notify_one(); // decoder_read_thread exits
	thread_decoder_read_thread->join();
	playback_lock_mutex.lock(); // Only when ao_play_thread is waiting
	playback_lock_mutex.unlock();
	play_next_condition.notify_one(); // ao_play_thread exits
	thread_ao_play_thread->join();
	dcerr("stopped");
}

LibAOBackend::~LibAOBackend() {
	dcerr("Shutting down");
	stop_output();
	ao_close(device);
	ao_shutdown();
	dcerr("Shut down");
}
