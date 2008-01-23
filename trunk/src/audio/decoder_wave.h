#ifndef DECODER_WAVE_H
#define DECODER_WAVE_H

#include "decoder_interface.h"
#include "datasource_interface.h"

class WaveDecoder : public IDecoder {
	private:
		IDataSource* source;
	public:
		WaveDecoder();
		WaveDecoder(AudioFormat af, IDataSource* source);
		~WaveDecoder();
		IDecoder* tryDecode(IDataSource* datasource);
		uint32 getData(uint8* buf, uint32 max);
};

#endif//DECODER_RAW_H
