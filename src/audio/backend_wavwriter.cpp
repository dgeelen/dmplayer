#include "backend_wavwriter.h"
#include "../error-handling.h"
#include <boost/bind.hpp>

WAVWriterBackend::WAVWriterBackend(AudioController* dec)	: IBackend(dec) {
	/* let the world know what audio format we accept */
	af.Channels      = 2;
	af.BitsPerSample = 16;
	af.SampleRate    = 44100;
	af.SignedSample  = true;
	af.LittleEndian  = true;

	f = fopen("output.wav", "wb");
	if(!f)
		throw Exception("Could not open file output.wav");

	done = false;
	decoder = dec;

	/* Start output thread */
	try {
		outputter_thread = new boost::thread(makeErrorHandler(boost::bind(&WAVWriterBackend::outputter, this)));
	}
	catch(...) {
		throw ThreadException("WAVWriterBackend: Could not start output thread!");
	}
}

void WAVWriterBackend::outputter() {
	uint8 buf[4096];
	while(!done) {
		int read = decoder->getData(buf, 4096);
		fwrite((char*)buf, sizeof(uint8), read, f);
	}
}

WAVWriterBackend::~WAVWriterBackend() {
	done = true;
	outputter_thread->join();
	fclose(f);
	delete outputter_thread;
}
