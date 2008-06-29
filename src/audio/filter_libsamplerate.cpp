#include "filter_libsamplerate.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <algorithm>

using namespace std;

LibSamplerateFilter::LibSamplerateFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	audioformat.SampleRate = target.SampleRate;
	int error = 0;
	samplerate_converter = src_new(/**/ SRC_SINC_BEST_QUALITY /*/ SRC_LINEAR /**/, src->getAudioFormat().Channels, &error);
	if(!samplerate_converter) {
		throw Exception(STRFORMAT("Could not initialize libsamplerate (error %i)", error));
	}
	samplerate_converter_state.end_of_input  = 0;
	samplerate_converter_state.src_ratio     = double(audioformat.SampleRate) / double(as->getAudioFormat().SampleRate);
}

LibSamplerateFilter::~LibSamplerateFilter() {
	src_delete(samplerate_converter);
}

/**
 * NOTE:
 *   Will not return less than one, or partial samples!
 */
uint32 LibSamplerateFilter::getData(uint8* buf, uint32 len)
{
	uint32 bytes_per_sample = audioformat.Channels * (audioformat.BitsPerSample / 8);
	uint32 nsamples_in      = uint32((double(len) / double(bytes_per_sample)) / samplerate_converter_state.src_ratio);
	uint32 nsamples_out     = len / bytes_per_sample;
	uint32 nsamples_in_len  = nsamples_in * bytes_per_sample;
	uint32 nsamples_out_len = nsamples_out * bytes_per_sample;
	float* data_in          = new float[nsamples_in*audioformat.Channels];
	float* data_out         = new float[nsamples_out*audioformat.Channels];
	uint8* data             = new uint8[nsamples_in_len];

	uint32 todo = nsamples_in_len;
	while(todo)
		todo-=src->getData(data + nsamples_in_len - todo, todo);

	uint8* ptr;
	ptr = data;
	for(uint32 i=0; i<(nsamples_in*audioformat.Channels); ++i) {
		switch(audioformat.BitsPerSample) {
			case 8: {
				if(audioformat.SignedSample) {
					data_in[i] = float(*((int8*)ptr))/255.0f;
				}
				else {
					throw Exception("TODO: Unsigned 8bit not supported"); //TODO
				}
				ptr += sizeof(uint8);
			}; break;
			case 16: {
				if(audioformat.SignedSample) {
					data_in[i] = float(*((int16*)ptr))/65535.0f;
				}
				else {
					throw Exception("TODO: Unsigned 16bit not supported"); //TODO
				}
				ptr += sizeof(uint16);
			}; break;
			case 32: {
				if(audioformat.SignedSample) {
					data_in[i] = float(*((int32*)ptr))/4294967295.0f;
				}
				else {
					throw Exception("TODO: Unsigned 32bit not supported"); //TODO
				}
				ptr += sizeof(uint32);
			}; break;
			default:
				throw Exception(STRFORMAT("Unsupported BitsPerSample for audioformat: %i", audioformat.BitsPerSample));
		}
	}

	samplerate_converter_state.data_in       = data_in;
	samplerate_converter_state.data_out      = data_out;
	samplerate_converter_state.input_frames  = nsamples_in;
	samplerate_converter_state.output_frames = nsamples_out;

	int error = src_process(samplerate_converter, &samplerate_converter_state);

	if(error) {
		throw Exception(STRFORMAT("Error while converting samplerate: %s", src_strerror(error)));
	}
	if((samplerate_converter_state.input_frames_used != nsamples_in) || (samplerate_converter_state.output_frames_gen != nsamples_out))
		dcerr("input_frames_used=" << samplerate_converter_state.input_frames_used << ", " <<
		      "nsamples_in=" << nsamples_in << ", " <<
		      "output_frames_gen=" << samplerate_converter_state.output_frames_gen << ", " <<
		      "nsamples_out="<<nsamples_out);

	ptr = buf;
	for(uint32 i=0; i<(nsamples_out*audioformat.Channels); ++i) {
		switch(audioformat.BitsPerSample) {
			case  8: {
				if(audioformat.SignedSample) {
					*(( int8*)ptr) =  int8(data_out[i]*255.0f*0.7f);
				}
				else {
					//TODO
				}
				ptr += sizeof(uint8);
			}; break;
			case 16: {
				if(audioformat.SignedSample) {
					*(( int16*)ptr) =  int16(data_out[i]*65535.0f);
				}
				else {
					//TODO
				}
				ptr += sizeof(uint16);
			}; break;
			case 32: {
				if(audioformat.SignedSample) {
					*(( int32*)ptr) =  int32(data_out[i]*4294967295.0f*0.7f);
				}
				else {
					//TODO
				}
				ptr += sizeof(uint32);
			}; break;
		}
	}

	delete data;
	delete data_in;
	delete data_out;
	return samplerate_converter_state.output_frames_gen * bytes_per_sample;
}

bool LibSamplerateFilter::exhausted() {
	return src->exhausted();
}
