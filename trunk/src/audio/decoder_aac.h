#ifndef DECODER_AAC_H
#define DECODER_AAC_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <faad.h>

#define BLOCK_SIZE 8*1024
class AACDecoder : public IDecoder {
	public:
		AACDecoder();
		~AACDecoder();
		IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 max);

	private:
		AACDecoder(IDataSourceRef source);
		IDataSourceRef datasource;

		uint32 buffer_fill;
		uint8 buffer[BLOCK_SIZE];
		uint32 sample_buffer_size;
		uint32 sample_buffer_index;
		uint8* sample_buffer;

		int aac_probe();
		void fill_buffer();

		//uint32 decoder_capabilities;
		faacDecHandle decoder_handle;
		faacDecConfigurationPtr decoder_config;

		void uninitialize();
		void initialize();
};

#endif//DECODER_AAC_H
