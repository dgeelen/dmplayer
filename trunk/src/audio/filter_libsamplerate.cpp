#include "filter_libsamplerate.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <algorithm>

using namespace std;

long int read_callback(void* _cb_data, float **data) {
	LibSamplerateFilter::CALLBACK_DATA* cb_data = (LibSamplerateFilter::CALLBACK_DATA*)_cb_data;

	uint32 bytes_per_frame = cb_data->channels * sizeof(float);
	uint32 nframes = cb_data->input_buffer.size() / bytes_per_frame;
	uint32 nbytes = nframes * bytes_per_frame;
	int read = cb_data->src->getData((uint8*)&(cb_data->input_buffer[0]), nbytes);

	(*data) = &(cb_data->input_buffer[cb_data->buffer_begin]);

	return (read) / bytes_per_frame;
}

LibSamplerateFilter::LibSamplerateFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	if(!src->getAudioFormat().Float) {
		throw Exception("Need float input");
	}
	audioformat.Float = true;
	audioformat.SampleRate = target.SampleRate;
	int error = 0;
	callback_data.input_buffer.resize(1024*32);
	callback_data.buffer_begin = 0;
	callback_data.buffer_end = 0;
	callback_data.src_state = src_callback_new(&read_callback,
	                                           SRC_SINC_BEST_QUALITY,
	                                           src->getAudioFormat().Channels,
	                                           &error,
	                                           &callback_data
	                                          );
	if(!callback_data.src_state) {
		throw Exception(STRFORMAT("Could not initialize libsamplerate (error %i)", error));
	}
	callback_data.channels = src->getAudioFormat().Channels;
	callback_data.src = src;
	is_exhausted = false;
}

LibSamplerateFilter::~LibSamplerateFilter() {
	src_delete(callback_data.src_state);
}

/**
 * NOTE:
 *   Will not return less than one, or partial samples!
 */
uint32 LibSamplerateFilter::getData(uint8* buf, uint32 len) {
	uint32 bytes_per_frame = callback_data.channels * sizeof(float);
	uint32 nframes         = len / bytes_per_frame;
	long read = src_callback_read(callback_data.src_state, double(audioformat.SampleRate) / double(src->getAudioFormat().SampleRate), nframes, (float*)buf) ;
	if(read==0) {
		int error = src_error(callback_data.src_state);
		if(error)
			throw Exception(STRFORMAT("Error while converting samplerate: %s", src_strerror(error)));
		is_exhausted = src->exhausted();
	}
	return read * bytes_per_frame;
}

bool LibSamplerateFilter::exhausted() {
	return is_exhausted;
}
