#ifndef FILTER_REFORMAT_H
#define FILTER_REFORMAT_H

#include "audiosource_interface.h"

class ReformatFilter : public IAudioSource {
	private:
		IAudioSource* src;
	public:
		ReformatFilter(IAudioSource* as, AudioFormat target);

		uint32 getData(uint8* buf, uint32 len);
};

#endif//FILTER_REFORMAT_H
