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
	af.Float         = false;
	decoder = dec;
	outputter_thread = NULL;
	f = NULL;
}

void WAVWriterBackend::start_output() {
	if(!outputter_thread) {
		done = false;
		/* Start output thread */
		try {
			outputter_thread = new boost::thread(makeErrorHandler(boost::bind(&WAVWriterBackend::outputter, this)));
		}
		catch(...) {
			throw ThreadException("WAVWriterBackend: Could not start output thread!");
		}
	}
}

void WAVWriterBackend::stop_output() {
	done = true;
	if(outputter_thread)
		outputter_thread->join();
	outputter_thread = NULL;
}

void WAVWriterBackend::outputter() {
	dcerr("starting WAV output");
	f = fopen("output.wav", "wb");
	if(!f)
		throw Exception("Could not open file output.wav");
	{
		uint32 ByteRate = af.SampleRate * af.Channels * af.BitsPerSample / 8;
		uint32 BlockAlign = af.Channels * af.BitsPerSample / 8;
		char hdr[48] = {'R', 'I', 'F', 'F',
		                  0,   0,   0,   0,
		                'W', 'A', 'V', 'E',
		                'f', 'm', 't', ' ',
		                 16,   0,   0,   0,
		                 af.Float ? 3 : 1,   0,
		                (af.Channels&0xff),   ((af.Channels>>8)&0xff),
		                (af.SampleRate&0xff), ((af.SampleRate>>8)&0xff), ((af.SampleRate>>16)&0xff), ((af.SampleRate>>24)&0xff),
		                (ByteRate&0xff), ((ByteRate>>8)&0xff), ((ByteRate>>16)&0xff), ((ByteRate>>24)&0xff),
		                (BlockAlign&0xff),   ((BlockAlign>>8)&0xff),
		                (af.BitsPerSample&0xff),   ((af.BitsPerSample>>8)&0xff),
		                'd', 'a', 't', 'a',
		                  0,   0,   0,   0,
		                  0,   0,   0,   0,
		               };
		fwrite(hdr, 1, 48, f);
	}

	uint32 total = 0;
	int size = (af.BitsPerSample>>3) * af.Channels * 1024;
	uint8* buf = new uint8[size];
	while (!done) {
		int read = decoder->getData(buf, size);
		fwrite((char*)buf, sizeof(uint8), read, f);
		total += read;
		if (read == 0) break;
	}
	delete[] buf;

	// FIXME: the values for total which we write are guestimates, 
	//        it seems to work ok though.
	//        (we might miss a few ms of data at the end)
	fseek(f, 8*4 + 4*2, SEEK_SET);
	fwrite(&total, 1, 4, f);
	total += 4 + 24 + 8;
	fseek(f, 4, SEEK_SET);
	fwrite(&total, 1, 4, f);
	fclose(f);
	dcerr("stopped WAV output");
}

WAVWriterBackend::~WAVWriterBackend() {
	dcerr("Shutting down");
	done = true;
	if(outputter_thread) {
		outputter_thread->join();
		delete outputter_thread;
		outputter_thread = NULL;
	}
	delete outputter_thread;
	dcerr("Shut down");
}
