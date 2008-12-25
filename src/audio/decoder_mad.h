#ifndef DECODER_MAD_H_INCLUDED
#define DECODER_MAD_H_INCLUDED

#include "decoder_interface.h"
#include "datasource_interface.h"
#include "mad.h"


// NOTE: INPUT_BUFFER_SIZE should be relatively large, some mp3's have really big ID3 tags due to embedding of album-covers etc.
#define INPUT_BUFFER_SIZE   (1024*16)
#define OUTPUT_BUFFER_SIZE  8192

class MadDecoder : public IDecoder {

	private:

		static signed short MadFixedToSshort(mad_fixed_t Fixed);
		uint8 input_buffer[INPUT_BUFFER_SIZE];
		uint8 output_buffer[OUTPUT_BUFFER_SIZE];

		mad_stream Stream;
		mad_frame Frame;
		mad_synth Synth;
		mad_timer_t Timer;
		char* ptr;

		bool eos;
		uint32 BytesInOutput;
		uint32 BytesInInput;

		IDataSourceRef datasource;

		void fill_buffer();
		MadDecoder(AudioFormat af, IDataSourceRef ds, size_t start_of_mp3_data_);

		size_t start_of_mp3_data;
	public:
		MadDecoder();
		~MadDecoder();
		IDecoderRef tryDecode(IDataSourceRef datasource);
		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
};

#endif // DECODER_MAD_H_INCLUDED
