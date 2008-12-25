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
	decoder = dec;
	outputter_thread = NULL;
	f = NULL;
}

void WAVWriterBackend::start_output() {
	done = false;
	/* Start output thread */
	try {
		outputter_thread = new boost::thread(makeErrorHandler(boost::bind(&WAVWriterBackend::outputter, this)));
	}
	catch(...) {
		throw ThreadException("WAVWriterBackend: Could not start output thread!");
	}
}

void WAVWriterBackend::stop_output() {
	done = true;
}

void WAVWriterBackend::outputter() {
	dcerr("starting WAV output");
	f = fopen("output.wav", "wb");
	if(!f)
		throw Exception("Could not open file output.wav");

	uint8 buf[4096];
	while(!done) {
		int read = decoder->getData(buf, 4096);
		fwrite((char*)buf, sizeof(uint8), read, f);
	}

	fclose(f);
	dcerr("stopped WAV output");
}

WAVWriterBackend::~WAVWriterBackend() {
	dcerr("Shutting down");
	done = true;
	if(outputter_thread) {
		outputter_thread->join();
		outputter_thread = NULL;
	}
	delete outputter_thread;
	dcerr("Shut down");
}
