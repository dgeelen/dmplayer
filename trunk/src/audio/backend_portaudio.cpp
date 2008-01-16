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
    float *out = (float*)outputBuffer;
    unsigned long i;
    int finished = 0;
    (void) outTime; /* Prevent unused variable warnings. */
    (void) inputBuffer;
    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = data->sine[data->left_phase];  /* left */
        *out++ = data->sine[data->right_phase];  /* right */
        data->left_phase += 1;
        if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
        data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
        if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
    }
    return finished;


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

    err = Pa_OpenStream(
              &stream,
              paNoDevice,/* default input device */
              0,              /* no input */
              paFloat32,  /* 32 bit floating point input */
              NULL,
              OUTPUT_DEVICE,
              2,          /* stereo output */
              paFloat32,      /* 32 bit floating point output */
              NULL,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              0,              /* number of buffers, if zero then use default minimum */
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";

    err = Pa_StartStream( stream );
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";
};