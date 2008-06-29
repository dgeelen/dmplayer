#include "backend_libao.h"
#include "../error-handling.h"
#include <boost/bind.hpp>

#define BUF_SIZE 1024*8 /* 8K buffers =~ 0.0464399sec of buffer) */
LibAOBackend::LibAOBackend(AudioController* dec)	: IBackend(dec), fill_buffer_barrier(2) {
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
	playing_buffer = 0;
	char x;
	for(int i=0; i<2; ++i) {
		audio_buffer[i] = new uint8[BUF_SIZE];
		for(int j=0; j<BUF_SIZE; ++j) {
			audio_buffer[i][j]= ++x;
		}
		x+=i;
	}

	decoder = dec;

	/* Start output thread */
	play_back = read_back = true;
	try {
		thread_decoder_read_thread = NULL;
		thread_ao_play_thread = NULL;
		thread_decoder_read_thread = new boost::thread(makeErrorHandler(boost::bind(&LibAOBackend::decoder_read_thread, this)));
		thread_ao_play_thread = new boost::thread(makeErrorHandler(boost::bind(&LibAOBackend::ao_play_thread, this)));
	}
	catch(...) {
		throw ThreadException("LibAOBackend: Could not start output thread!");
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
	long todo;
	while(read_back) {
		todo = BUF_SIZE;
		while(todo>0)
			if(decoder)
				todo -= decoder->getData(audio_buffer[0] + (BUF_SIZE - todo), todo);
		fill_buffer_barrier.wait();

		todo = BUF_SIZE;
		while(todo>0)
			if(decoder)
				todo -= decoder->getData(audio_buffer[1] + (BUF_SIZE - todo), todo);
		fill_buffer_barrier.wait();
	}
}

void LibAOBackend::ao_play_thread() {
	while(play_back) {
		fill_buffer_barrier.wait();
		ao_play( device, (char*)audio_buffer[0], BUF_SIZE);
		fill_buffer_barrier.wait();
		ao_play( device, (char*)audio_buffer[1], BUF_SIZE);
	}
	fill_buffer_barrier.wait();
}

LibAOBackend::~LibAOBackend() {
	play_back = false;
	if (thread_ao_play_thread) thread_ao_play_thread->join();
	read_back = false;
	fill_buffer_barrier.wait();
	if (thread_decoder_read_thread) thread_decoder_read_thread->join();
	ao_close(device);
	ao_shutdown();
}
