#include "backend_portaudio.h"
#include "../error-handling.h"
#include "../util/StrFormat.h"
#include <boost/thread/mutex.hpp>
boost::mutex pa_mutex; // PortAudio is not thread safe

#define BUFFER_SECONDS (1) // FIXME: This is really coarse grained

using namespace std;

int PortAudioBackend::pa_callback(const void *inputBuffer, void *outputBuffer,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData )
{
	PortAudioBackend* be = (PortAudioBackend*)userData;
	uint8* out = (uint8*)outputBuffer;
	unsigned long byteCount = (frameCount * be->af.Channels * be->af.BitsPerSample) >> 3; // divide by 8 for bits->bytes

	return be->data_callback(out, byteCount);
}

int PortAudioBackend::data_callback(uint8* out, unsigned long byteCount)
{
	// FIXED: This callback runs with PRIO_REALTIME, but it doesn't do much so this won't cause problems
	//        only 1 mutex is locked, 1 memcpy call is made, and possibly 1 memset call
	//        in addition the mutex is not held by any other threads while they
	//        do anything taking more than a few cycles

	size_t read = cbuf.read(out, byteCount);

	if (read < byteCount)
		memset(out+read, 0, byteCount-read);

	return 0;
}

void PortAudioBackend::decodeloop()
{
	try {
		size_t decode = 0;
		uint8* buf = NULL;
		while (true) {
			boost::this_thread::interruption_point();
			cbuf.write(buf, decode, decode);

			decode = dec->getData(buf, decode);

			if (decode == 0) boost::this_thread::sleep(boost::posix_time::milliseconds(10)); 
		}
	} catch (boost::thread_interrupted& /*e*/) {
	}
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
	af.Channels      = n_channels >= 6 ? 6 : 2;
	af.BitsPerSample = 32;
	af.SignedSample  = true;
	af.LittleEndian  = true;
	af.Float         = true;

	cbuf.reset(BUFFER_SECONDS * (af.BitsPerSample/8) * af.Channels * af.SampleRate);

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
	stop_output();

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

	decodethread.interrupt();
	decodethread.join();
}

void PortAudioBackend::start_output()
{
	boost::mutex::scoped_lock lock(pa_mutex);
	PaError err = Pa_StartStream( stream );
	if ((err != paNoError) && (err != paStreamIsNotStopped))
		throw SoundException(STRFORMAT("failed to start portaudio stream: %s", Pa_GetErrorText(err)));

	decodethread = boost::thread(&PortAudioBackend::decodeloop, this).move();
}
