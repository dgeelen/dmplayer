#ifndef DECODER_WAVE_H
#define DECODER_WAVE_H

#include "decoder_interface.h"
#include "datasource_interface.h"

class WaveDecoder : public IDecoder {
	private:
		IDataSourceRef source;
	public:
		WaveDecoder(AudioFormat af, IDataSourceRef source);
		~WaveDecoder();
		static IDecoderRef tryDecode(IDataSourceRef datasource);
		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
};

#endif//DECODER_RAW_H
