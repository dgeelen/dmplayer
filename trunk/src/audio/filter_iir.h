#ifndef FILTER_IIR_H
#define FILTER_IIR_H

#include "audiosource_interface.h"
#include <vector>
#include <deque>

class IIRFilter : public IAudioSource {
	public:
		IIRFilter(IAudioSourceRef as, AudioFormat target, std::vector<float> feedforward_coefficients, std::vector<float> feedback_coefficients);
		~IIRFilter();
		uint32 getData(uint8* buf, uint32 len);
		bool exhausted();

	private:
		IAudioSourceRef src;
		bool is_exhausted;
		std::vector<float> feedforward_coefficients;
		std::vector<float> feedback_coefficients;
		std::deque<float> inputs;
		std::deque<float> outputs;
};
#endif //FILTER_IIR_H
