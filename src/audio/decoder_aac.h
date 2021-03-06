#ifndef DECODER_AAC_H
#define DECODER_AAC_H

#include "decoder_interface.h"
#include "datasource_interface.h"

#include "../util/ScopeExitSignal.h"

#include <faad.h>
#ifdef __NEAACDEC_H__
	// yes this is crappy, but there is no other suitable define it seems
	// and the define for faacDecGetCapabilities is missing in faad 2.6
	#ifndef faacDecGetCapabilities
		#define faacDecGetCapabilities NeAACDecGetCapabilities
		#define FAAD_UINT32TYPE uint32
	#else
		#define FAAD_UINT32TYPE uint32_t
	#endif
#endif

class AACDecoder : public IDecoder {
	public:
		~AACDecoder();
		static IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
	private:
		static const size_t BLOCK_SIZE = 8*1024;
		AACDecoder(IDataSourceRef source);

		void uninitialize();
		void initialize();

		int aac_probe();
		void fill_buffer();

		IDataSourceRef datasource;

		uint32 buffer_fill;
		uint8 buffer[BLOCK_SIZE];
		uint32 sample_buffer_size;
		uint32 sample_buffer_index;
		uint8* sample_buffer;

		uint32 decoder_capabilities;
		faacDecHandle decoder_handle;
		faacDecConfigurationPtr decoder_config;

		ScopeExitSignal onDestroy; // should be last data member
};

#endif//DECODER_AAC_H
