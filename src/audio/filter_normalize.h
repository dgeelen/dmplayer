#ifndef FILTER_NORMALIZE_H
#define FILTER_NORMALIZE_H

#include "audiosource_interface.h"
#include "filter_iir.h"
#include "filter_split.h"
#include <boost/shared_ptr.hpp>
#include <boost/circular_buffer.hpp>
#include <deque>
#include <vector>
#include <set>

class NormalizeFilter : public IAudioSource {
	public:
		NormalizeFilter(IAudioSourceRef as, AudioFormat target);
		~NormalizeFilter();
		bool exhausted();
		uint32 getData(uint8* buf, uint32 len);
	private:
		void update_gain();
		boost::shared_ptr<SplitFilter> splitter;
		boost::shared_ptr<IIRFilter  > IIRYule;
		boost::shared_ptr<IIRFilter  > IIRButter;
		std::set<float> dB_map;
		float peak_amplitude, gain;
		uint32 position;
		IAudioSourceRef src;
		uint32 rms_period_sample_count;
};

#endif //FILTER_NORMALIZE_H
