#include "filter_iir.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"

using namespace std;

/**
 * Let n = max(feedforward_coefficients.size(), feedbackward_coefficients.size()).
 * IIR Filter guarantees that is it delays input by exactly n samples. The first n
 * output samples are to be considered noise. IIR Filter is not exhausted until all
 * input samples have passed through the filter.
 */
IIRFilter::IIRFilter(IAudioSourceRef as, AudioFormat target, std::vector<float> feedforward_coefficients, std::vector<float> feedback_coefficients)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	// Ensure there is at least one element in the coefficients
	if(feedforward_coefficients.size() == 0)
		feedforward_coefficients.push_back(1.0);

	// Extend coefficients with 0's to equal size so that both arrays have the same size (important in getData)
	uint32 ff_size = feedforward_coefficients.size();
	uint32 fb_size = feedback_coefficients.size();
	feedforward_coefficients.resize(std::max(ff_size, fb_size));
	feedback_coefficients.resize(std::max(ff_size, fb_size));

	// Prevent possible division by zero in getData
	if(feedback_coefficients[0] == 0.0)
		feedback_coefficients[0] = 1.0;

	this->feedforward_coefficients = feedforward_coefficients;
	this->feedback_coefficients = feedback_coefficients;
	this->inputs  = std::deque<float>(feedforward_coefficients.size() * audioformat.Channels);
	this->outputs = std::deque<float>(feedback_coefficients.size() * audioformat.Channels);
	audioformat.Float = true;
	is_exhausted = false;
}

IIRFilter::~IIRFilter() {
}

uint32 IIRFilter::getData(uint8* buf, uint32 len) {
	uint32 bytes_per_frame = audioformat.Channels * sizeof(float);
	uint32 nframes         = len / bytes_per_frame;
	uint32 nfwdcoef        = feedforward_coefficients.size();
	uint32 ninputframes    = inputs.size() / audioformat.Channels;
	uint32 noutputframes   = outputs.size() / audioformat.Channels;
	uint32 todo            = nframes;

	if(ninputframes < nframes + nfwdcoef) { // ninputframes - nfwdcoef < nframes
		uint32 toread = audioformat.Channels * (nframes + nfwdcoef - ninputframes) * sizeof(float);
		vector<float> b(toread);
		uint32 read = src->getData((uint8*)&b[0], toread);
		inputs.insert(inputs.end(), b.begin(), b.begin()+(read / sizeof(float)));
		if(read < toread) {
			if(src->exhausted() && !is_exhausted) {
				inputs.resize(inputs.size() + (nfwdcoef * audioformat.Channels)); // add some fake input
				is_exhausted = true;
			}
		}
		ninputframes = inputs.size() / audioformat.Channels;
	}

	float output[audioformat.Channels];
	for(uint32 i = nfwdcoef - 1; (i < ninputframes) && (todo > 0); ++i) {
	 	// Note: We try to access elements in inputs in-order to optimize memory access. Does this help?
		for(uint32 chan = 0 ; chan < audioformat.Channels; ++chan) {
			output[chan] = feedforward_coefficients[0] * inputs[i*audioformat.Channels + chan];
		}
		for(uint32 coef = 1; coef < nfwdcoef; ++coef) { // Process elements side-by side to minimize chances of overflow
			for(uint32 chan = 0; chan < audioformat.Channels; ++chan) {
				float inp     = inputs[(i-coef)*audioformat.Channels + chan];
				float outp    = outputs[(noutputframes - coef) * audioformat.Channels + chan];
				float ffc     = feedforward_coefficients[coef];
				float fbc     = feedback_coefficients[coef];
				output[chan] += ffc * inp - fbc * outp;
			}
		}
		for(uint32 chan = 0 ; chan < audioformat.Channels; ++chan) {
			output[chan] /= feedback_coefficients[0];
			outputs.push_back( output[chan] );
		}
		noutputframes++;
		--todo;
	}

	for(uint32 i=0; i < (nframes - todo); ++i) { //TODO Optimize
		for(uint32 c = 0; c < audioformat.Channels; ++c) {
			((float*)buf)[i*audioformat.Channels + c] = outputs[i*audioformat.Channels + c];
		}
	}
	inputs.erase(inputs.begin(), inputs.begin() + (nframes - todo) * audioformat.Channels );
	outputs.erase(outputs.begin(), outputs.begin() + (nframes - todo) * audioformat.Channels);
	return (nframes - todo) * bytes_per_frame;
}

bool IIRFilter::exhausted() {
	return is_exhausted && (inputs.size() == ((feedforward_coefficients.size()-1) * audioformat.Channels));
}
