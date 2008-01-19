#include "backend_portaudio.h"

#define SAMPLE_RATE   (44100/2)
#define FRAMES_PER_BUFFER  (64)
#define OUTPUT_DEVICE Pa_GetDefaultOutputDeviceID()

static int patestCallback(   void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             PaTimestamp outTime, void *userData )
{
	IDecoder* decoder = (IDecoder*)userData;
	char *out = (char*)outputBuffer;

	// 2 byte samples + 2 channels -> 2*2=4 -> 4 bytes/frame
	uint32 act = decoder->doDecode(out, framesPerBuffer*4, framesPerBuffer*4);
	return 0;
}

PortAudioBackend::PortAudioBackend(IDecoder* dec)
	: IBackend(dec)
{
	PaError err = Pa_Initialize();
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";

	err = Pa_OpenStream(
		&stream,
		paNoDevice,        /* default input device */
		0,                 /* no input */
		paInt16,           /* 16 bit signed input */
		NULL,
		OUTPUT_DEVICE,
		2,                 /* stereo output */
		paInt16,           /* 16 bit signed output */
		NULL,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		0,                 /* number of buffers, if zero then use default minimum */
		paClipOff,         /* we won't output out of range samples so don't bother clipping them */
		patestCallback,
		dec );

	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";

	err = Pa_StartStream( stream );
	if (err != paNoError) throw "error! PortAudioBackend::PortAudioBackend";
};

PortAudioBackend::~PortAudioBackend()
{
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	Pa_Terminate();
}
