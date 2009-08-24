#include "backend_portaudio.h"
#include "../error-handling.h"
#include "../util/StrFormat.h"

#define SAMPLE_RATE    (44100)
#define FRAMES_PER_BUFFER (1024*4)  /* number of frames(=samples) per buffer */
#define NUM_BUFFERS        (2)  /* number of buffers, if zero then use default minimum */

#if PA_VERSION == 18
static int pa_callback( void *inputBuffer, void *outputBuffer,
                        unsigned long frameCount,
                        PaTimestamp outTime, void *userData )
#endif
#if PA_VERSION == 19
static int pa_callback(const void *inputBuffer, void *outputBuffer,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData )
#endif
{	// FIXME: This callback runs with PRIO_REALTIME, and can potentially
	//        halt the system if data is not provided at an adequate pace.
	//        (such as happens for example at the end of the getData()
	//         chain, which is connected to the network...)
	AudioController* ac = (AudioController*)userData;
	char *out = (char*)outputBuffer;

	// 2 byte samples + 2 channels -> 2*2=4 -> 4 bytes/frame
	uint32 act = ac->getData((uint8*)out, frameCount*4);
	return 0;
}

PortAudioBackend::PortAudioBackend(AudioController* dec)
	: IBackend(dec)
{
	PaError err = Pa_Initialize();
	if (err != paNoError) throw SoundException("PortAudioBackend initialisation failed");

	af.SampleRate = SAMPLE_RATE;
	af.Channels = 2;
	af.BitsPerSample = 16;
	af.SignedSample = true;
	af.LittleEndian = true;

#if PA_VERSION == 18
	err = Pa_OpenStream(
		&stream,
		paNoDevice,        /* no input device */
		0,                 /* no input */
		paInt16,           /* 16 bit signed input */
		NULL,
		Pa_GetDefaultOutputDeviceID(),
		2,                 /* stereo output */
		paInt16,           /* 16 bit signed output */
		NULL,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		NUM_BUFFERS,
		paClipOff,         /* we won't output out of range samples so don't bother clipping them */
		pa_callback,
		dec );
#endif
#if PA_VERSION == 19
	PaStreamParameters outparams;
	outparams.channelCount = 2;
	outparams.device = Pa_GetDefaultOutputDevice();
	outparams.sampleFormat = paInt16;
	outparams.hostApiSpecificStreamInfo = NULL;
	outparams.suggestedLatency = 0.2;
	err = Pa_OpenStream(
		&stream,
		NULL,
		&outparams,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		0,
		pa_callback,
		dec );
#endif

	if (err != paNoError) throw SoundException("failed to open portaudio stream");
};

PortAudioBackend::~PortAudioBackend()
{
	dcerr("shutting down");
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	Pa_Terminate();
	dcerr("shut down");
}

void PortAudioBackend::stop_output()
{
	PaError err = Pa_StopStream( stream );
	if((err != paNoError) && (err != paStreamIsStopped))
		throw SoundException(STRFORMAT("failed to stop portaudio stream: %s", Pa_GetErrorText(err)));
}

void PortAudioBackend::start_output()
{
	PaError err = Pa_StartStream( stream );
	if ((err != paNoError) && (err != paStreamIsNotStopped))
		throw SoundException(STRFORMAT("failed to start portaudio stream: %s", Pa_GetErrorText(err)));
}

