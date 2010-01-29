#include "backend_portaudio.h"
#include "../error-handling.h"
#include "../util/StrFormat.h"
#include <boost/thread/mutex.hpp>
boost::mutex pa_mutex; // PortAudio is not thread safe

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
	boost::mutex::scoped_lock lock(pa_mutex);
	PaError err = Pa_Initialize();
	if (err != paNoError)
		throw SoundException("PortAudioBackend initialisation failed");

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
	double latency     = deviceInfo->defaultHighOutputLatency;
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
		outputParameters.suggestedLatency = deviceInfo->defaultHighOutputLatency;
		if(Pa_IsFormatSupported(NULL, &outputParameters, deviceInfo->defaultSampleRate) == paFormatIsSupported) {
			if( ((deviceInfo->maxOutputChannels > n_channels) &&  (deviceInfo->defaultSampleRate >= samplerate)))
			{
				best_device = i;
				n_channels = deviceInfo->maxOutputChannels;
				samplerate = deviceInfo->defaultSampleRate;
				latency    = deviceInfo->defaultHighOutputLatency;
			}
			dcerr("device supports " << deviceInfo->maxOutputChannels << " channels, " << deviceInfo->defaultSampleRate << "Hz, Float32 output, with " << deviceInfo->defaultHighOutputLatency << "s latency");
		}
		else {
			dcerr("device does not support " << deviceInfo->maxOutputChannels << " channels, " << deviceInfo->defaultSampleRate << "Hz, Float32 output, with " << deviceInfo->defaultHighOutputLatency << "s latency");
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
	// Request 1/5 of a second output latency (should give us 'interactive enough'
	// performance while still allowing some other tasks to interrupt playback
	// momentarily)
	outputParameters.suggestedLatency          = 0.2;

	if(Pa_IsFormatSupported(NULL, &outputParameters, samplerate) == paFormatIsSupported) {
		if(Pa_OpenStream(
			&stream,
			NULL,
			&outputParameters,
			samplerate,
			paFramesPerBufferUnspecified, // Let PortAudio decide the optimal number of frames per buffer
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
	boost::mutex::scoped_lock lock(pa_mutex);
	dcerr("shutting down");
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	Pa_Terminate();
	dcerr("shut down");
}

void PortAudioBackend::stop_output()
{
	boost::mutex::scoped_lock lock(pa_mutex);
	PaError err = Pa_StopStream( stream );
	if((err != paNoError) && (err != paStreamIsStopped))
		throw SoundException(STRFORMAT("failed to stop portaudio stream: %s", Pa_GetErrorText(err)));
}

void PortAudioBackend::start_output()
{
	boost::mutex::scoped_lock lock(pa_mutex);
	PaError err = Pa_StartStream( stream );
	if ((err != paNoError) && (err != paStreamIsNotStopped))
		throw SoundException(STRFORMAT("failed to start portaudio stream: %s", Pa_GetErrorText(err)));
}
