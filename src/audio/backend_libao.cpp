#include "backend_libao.h"
#include "../error-handling.h"
#include <boost/bind.hpp>

#define BUF_SIZE 1024*8 /* 8K buffers =~ 0.0464399sec of buffer) */
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
			audio_buffer[i][j]= 0;
		}
	}

	decoder = dec;

	/* Start output thread */
	fill_buffer_barrier = new boost::barrier::barrier(2);
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
 *   Mutexen mutex_a and mutex_b to represent each buffer
 *   Locks lock_a and lock_b to lock buffers A and B for use by a particular thread
 *   The barrier ensures that both threads are only active when there is something
 *   which needs doing.
 *     Initially:
 *       A is empty, B is empty, N=0
 *     Then:
 *       Before playing A, the playback thread waits                (N=1, A empty,          B empty         )
 *       After filling A, the reader waits                          (N=2, A full,           B empty         )
 *   (#) N=2 -> Now both continue: playback plays A, reader reads B (N=0, A emptying,       B filling       )
 *       Before playing B, the playback thread waits                (N=1, A empty,          B filling/full  )
 *       After filling B, the reader waits                          (N=2, A empty/emptying, B full          )
 *       N=2 -> Now both continue: playback plays B, reader reads A (N=0, A filling,        B emptying      )
 *       Before playing A, the playback thread waits                (N=1, A filling/full,   B empty         )
 *       After filling A, the reader waits                          (N=2, A full,           B empty/emptying)
 *       goto (#)

 */
void LibAOBackend::decoder_read_thread() {
	int filled_buffer = 1-playing_buffer;
	while(play_back) {
		{ //Fill buffer a
			boost::mutex::scoped_lock lk_a( buffer_a_mutex ); // After this we have buffer A
			if(decoder)
				audio_buffer_fill[0] = decoder->doDecode(audio_buffer[0], BUF_SIZE, BUF_SIZE);
		}
		fill_buffer_barrier->wait();
		{ //Fill buffer b
			boost::mutex::scoped_lock lk_b( buffer_b_mutex ); // After this we have buffer B
			if(decoder)
				audio_buffer_fill[1] = decoder->doDecode(audio_buffer[1], BUF_SIZE, BUF_SIZE);
		}
		fill_buffer_barrier->wait();
	}
}

void LibAOBackend::ao_play_thread() {
	while(play_back) {
		fill_buffer_barrier->wait();
		{ //Play buffer a
			boost::mutex::scoped_lock lk_a( buffer_a_mutex ); // After this we have buffer A
			ao_play( device, audio_buffer[0], audio_buffer_fill[0]);
		}
		fill_buffer_barrier->wait();
		{ //Play buffer b
			boost::mutex::scoped_lock lk_b( buffer_b_mutex ); // After this we have buffer B
			ao_play( device, audio_buffer[1], audio_buffer_fill[1]);
		}
	}
}

LibAOBackend::~LibAOBackend() {
	play_back=false;
	if (thread_ao_play_thread) thread_ao_play_thread->join();
	if (thread_decoder_read_thread) thread_decoder_read_thread->join();
	delete fill_buffer_barrier;
	ao_close(device);
	ao_shutdown();
}
