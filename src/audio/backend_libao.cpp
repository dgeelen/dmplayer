#include "backend_libao.h"
#include "../error-handling.h"
#include <boost/bind.hpp>

#define BUF_SIZE 1024*160
LibAOBackend::LibAOBackend(IDecoder* dec)	: IBackend(dec) {
	ao_initialize();
	int default_driver = ao_default_driver_id();
	format.bits = 16;
	format.channels = 2;
	format.rate = 44100;
	format.byte_format = AO_FMT_LITTLE;
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		throw "LibAOBackend: Error opening device!";
	}

	/* Initialize playback buffers, we use double buffering */
	playing_buffer = 0;
	char x;
	for(int i=0; i<2; ++i) {
		audio_buffer[i] = new char[BUF_SIZE];
		audio_buffer_fill[i] = BUF_SIZE;
		for(int j=0; j<BUF_SIZE; ++j) {
			audio_buffer[i][j]= ++x;
			x+=i;
		}
	}

	/* Start output thread */
	play_back = true;
	try {
		thread_decoder_read_thread = NULL;
		thread_ao_play_thread = NULL;
		thread_decoder_read_thread = new boost::thread(error_handler(boost::bind(&LibAOBackend::decoder_read_thread, this)));
		thread_ao_play_thread = new boost::thread(error_handler(boost::bind(&LibAOBackend::ao_play_thread, this)));
	}
	catch(...) {
		throw "LibAOBackend: Could not start output thread!";
	}
}


/*
 *   mutexen mutex_a and mutex_b to represent each buffer
 *   locks lock_a and lock_b to lock buffers A and B for use by a particular thread
 *   conditions cond_a and cond_b to indicate A/B will be released by the opposing thread
 *
 */
void LibAOBackend::decoder_read_thread() {
	int filled_buffer = 1-playing_buffer;
	dcerr("start");
	while(play_back) {
		{ //Fill buffer a
			dcerr("waiting on lock for buffer a");
			boost::mutex::scoped_lock lk_a( buffer_a_mutex ); // After this we have buffer A
			dcerr("Filling buffer a");

			buffer_b_condition.wait(lk_a);   // Wait until playback thread releases playing B
			buffer_a_condition.notify_all(); // Notify playback thread that it can play A
		}
		{ //Fill buffer b
			dcerr("waiting on lock for buffer b");
			boost::mutex::scoped_lock lk_b( buffer_b_mutex ); // After this we have buffer B
			dcerr("Filling buffer b");
			usleep(100000); //'fill it'
			buffer_a_condition.wait(lk_b);   // Wait until playback thread releases playing A
			buffer_b_condition.notify_all(); // Notify playback thread that it can play B
		}
	}
	dcerr("stop");
}

void LibAOBackend::ao_play_thread() {
// 	uint32 act = data->decoder->doDecode(out, framesPerBuffer*4, framesPerBuffer*4);
	dcerr("start");
	while(play_back) {

		{ //Play buffer b
			dcerr("waiting on lock for buffer b"); // After this we have buffer B
			boost::mutex::scoped_lock lk_b( buffer_b_mutex );
			dcerr("Playing buffer b");
			ao_play( device, audio_buffer[1], audio_buffer_fill[1]); // Play it
			dcerr("Played buffer b");
			buffer_b_condition.notify_all(); // Notify reader thread it can refill B
			buffer_a_condition.wait(lk_b);   // Wait until reader thread finishes reading A
		}
		{ //Play buffer a
			dcerr("waiting on lock for buffer a");
			boost::mutex::scoped_lock lk_a( buffer_a_mutex ); // After this we have buffer A
			dcerr("Playing buffer a");
			ao_play( device, audio_buffer[0], audio_buffer_fill[0]); // Play it
			dcerr("Played buffer a");
			buffer_a_condition.notify_all(); // Notify reader thread it can refill A
			buffer_b_condition.wait(lk_a);   // Wait until reader thread finishes reading B
		}
	}
	dcerr("stop");
}

LibAOBackend::~LibAOBackend() {
	play_back=false;
	if (thread_ao_play_thread) thread_ao_play_thread->join();
	if (thread_decoder_read_thread) thread_decoder_read_thread->join();
	ao_close(device);
	ao_shutdown();
}
