#ifndef DECODER_MAD_H_INCLUDED
#define DECODER_MAD_H_INCLUDED

#include "decoder_interface.h"
#include "datasource_interface.h"
#include "mad.h"


#define INPUT_BUFFER_SIZE	(5*8192)
#define OUTPUT_BUFFER_SIZE	8192 /* Must be an integer multiple of 4. */

class MadDecoder : public IDecoder{

	private:

		static signed short MadFixedToSshort(mad_fixed_t Fixed);
		char* INPUT_BUFFER;
		char* OUTPUT_BUFFER;

		mad_stream Stream;
		mad_frame Frame;
		mad_synth Synth;
		mad_timer_t Timer;
		char* ptr;

		bool eos;
		uint32 BytesInOutput;
		uint32 BytesInInput;

		IDataSource* datasource;

		MadDecoder(IDataSource* ds);
	public:
		MadDecoder();
		~MadDecoder();
		IDecoder* tryDecode(IDataSource* datasource);
		uint32 doDecode(uint8* buf, uint32 max, uint32 req);
};

#endif // DECODER_MAD_H_INCLUDED
