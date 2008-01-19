#include "backend_portaudio.h"
#include <cstdlib>

#define NUM_SECONDS   (10)
#define SAMPLE_RATE   (44100)
#define AMPLITUDE     (0.9)
#define FRAMES_PER_BUFFER  (64)
#define OUTPUT_DEVICE Pa_GetDefaultOutputDeviceID()
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#include <math.h>

static int patestCallback(   void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData*)userData;
    char *out = (char*)outputBuffer;

	uint32 act = data->decoder->doDecode(out, framesPerBuffer, framesPerBuffer);
	//for (uint32 i = 0; i < act; i+=2) {
	//	char tmp;
	//	tmp = out[i];
	//	out[i] = out[i+1];
	//	out[i+1] = tmp;
	//}
	return 0;
}


PortAudioBackend::PortAudioBackend(IDecoder* dec)
	: IBackend(dec)
{
	PaError err = Pa_Initialize();
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";

    /* initialise sinusoidal wavetable */
    for( int i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) (AMPLITUDE * sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. ));
    }
    data.left_phase = data.right_phase = 0;
	data.decoder = dec;

    err = Pa_OpenStream(
              &stream,
              paNoDevice,/* default input device */
              0,              /* no input */
              paInt16,  /* 32 bit floating point input */
              NULL,
              OUTPUT_DEVICE,
              2,          /* stereo output */
              paInt16,      /* 32 bit floating point output */
              NULL,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
			  100,              /* number of buffers, if zero then use default minimum */
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";

    err = Pa_StartStream( stream );
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";
};

PortAudioBackend::~PortAudioBackend()
{
	PaError err = Pa_StopStream(stream);
	Pa_Terminate();
}
