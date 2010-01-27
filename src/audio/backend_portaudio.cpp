#include "backend_portaudio.h"
#include "../error-handling.h"
#include "../util/StrFormat.h"

using namespace std;

int PortAudioBackend::pa_callback(const void *inputBuffer, void *outputBuffer,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData )
{	// FIXME: This callback runs with PRIO_REALTIME, and can potentially
	//        halt the system if data is not provided at an adequate pace.
	//        (such as happens for example at the end of the getData()
	//         chain, which is connected to the network...)
	PortAudioBackend* be = (PortAudioBackend*)userData;
	char *out = (char*)outputBuffer;

	// Float32 is 4 bytes, so #channels * 4 = bytes/frame
	uint32 act = be->dec->getData((uint8*)out, frameCount*(be->af.Channels)*4);
	return 0;
}

PortAudioBackend::PortAudioBackend(AudioController* _dec)
	: IBackend(dec), dec(_dec)
{
	PaError err = Pa_Initialize();
	if (err != paNoError) throw SoundException("PortAudio backend: ERROR: initialisation failed");

	int numDevices = Pa_GetDeviceCount();
	if(numDevices < 0) {
		throw SoundException(STRFORMAT("PortAudio backend: ERROR: Pa_GetDeviceCount returned 0x%x (%s)\n", numDevices, Pa_GetErrorText(numDevices)));
	}
	if(numDevices == 0) {
		throw SoundException("PortAudio backend: ERROR: No output devices available");
	}
	const   PaDeviceInfo *deviceInfo;
	int default_device = Pa_GetDefaultOutputDevice();
	deviceInfo = Pa_GetDeviceInfo( default_device );
	int best_device    = default_device;
	int n_channels     = deviceInfo->maxOutputChannels;
	double samplerate  = deviceInfo->defaultSampleRate;
	double latency     = deviceInfo->defaultLowOutputLatency;
	for(int i=0; i<numDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo( i );
		dcerr( "Found device #" << i << ": '" << deviceInfo->name << "'");
		dcerr( "\tmaxInputChannels=" << deviceInfo->maxInputChannels);
		dcerr( "\tmaxOutputChannels=" << deviceInfo->maxOutputChannels);
		dcerr( "\tdefaultSampleRate=" << deviceInfo->defaultSampleRate);

		PaStreamParameters outputParameters;
		outputParameters.channelCount = deviceInfo->maxOutputChannels;
		outputParameters.device = i;
		outputParameters.hostApiSpecificStreamInfo = NULL;
		outputParameters.sampleFormat = paFloat32;
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
		if(Pa_IsFormatSupported(NULL, &outputParameters, deviceInfo->defaultSampleRate) == paFormatIsSupported) {
			if( ((deviceInfo->maxOutputChannels > n_channels) &&  (deviceInfo->defaultSampleRate >= samplerate)))
			{
				best_device = i;
				n_channels = deviceInfo->maxOutputChannels;
				samplerate = deviceInfo->defaultSampleRate;
				latency    = deviceInfo->defaultLowOutputLatency;
			}
			dcerr("device supports " << deviceInfo->maxOutputChannels << " channels, " << deviceInfo->defaultSampleRate << "Hz, Float32 output, with " << deviceInfo->defaultLowOutputLatency << "s latency");
		}
		else {
			dcerr("device does not support " << deviceInfo->maxOutputChannels << " channels, " << deviceInfo->defaultSampleRate << "Hz, Float32 output, with " << deviceInfo->defaultLowOutputLatency << "s latency");
		}
	}
	dcerr("Selecting device #" << best_device << " (default is #" << default_device << ")");
	
	if(fabs(samplerate - double(long(samplerate))) > 0.5)
		cout << "PortAudio backend: WARNING: samplerate mismatch > 0.5!" << endl;

	af.SampleRate    = uint32(samplerate);
	af.Channels      = n_channels > 6 ? 6 : 2;
	af.BitsPerSample = 32;
	af.SignedSample  = true;
	af.LittleEndian  = true;
	af.Float         = true;

	PaStreamParameters outputParameters;
	outputParameters.channelCount              = af.Channels;
	outputParameters.device                    = best_device;
	outputParameters.sampleFormat              = paFloat32;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	outputParameters.suggestedLatency          = latency;

	if(Pa_IsFormatSupported(NULL, &outputParameters, samplerate) == paFormatIsSupported) {
		if(Pa_OpenStream(
			&stream,
			NULL,
			&outputParameters,
			samplerate,
			1024 * outputParameters.channelCount * 4, /* Float32 is 4 bytes */
			paClipOff,         /* we won't output out of range samples so don't bother clipping them */
			pa_callback,
			(void*)this)
		!= paNoError)
			throw SoundException("PortAudio backend: ERROR: failed to open portaudio stream");
	}
	else {
		throw SoundException("PortAudio backend: ERROR: Format not supported!");
	}
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

