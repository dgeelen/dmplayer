#ifndef DECODER_WAVE_H
#define DECODER_WAVE_H

#include "decoder_interface.h"
#include "datasource_interface.h"

class WaveDecoder : public IDecoder {
	private:
		IDataSourceRef source;
	public:
		WaveDecoder();
		WaveDecoder(AudioFormat af, IDataSourceRef source);
		~WaveDecoder();
		IDecoderRef tryDecode(IDataSourceRef datasource);
		uint32 getData(uint8* buf, uint32 max);
};

#endif//DECODER_RAW_H
