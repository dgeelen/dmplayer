#ifndef DECODER_MAD_H_INCLUDED
#define DECODER_MAD_H_INCLUDED

#include "decoder_interface.h"
#include "datasource_interface.h"

#define INPUT_BUFFER_SIZE	(5*8192)
#define OUTPUT_BUFFER_SIZE	8192 /* Must be an integer multiple of 4. */

class MadDecoder : public IDecoder{

	private:
		char* INPUT_BUFFER;
		char* OUTPUT_BUFFER[OUTPUT_BUFFER_SIZE];
	public:
		MadDecoder();
		~MadDecoder();
		IDecoder* tryDecode(IDataSource* datasource);
		uint32 doDecode(uint8* buf, uint32 max, uint32 req) { return 0; }; //FIXME: implement in .cpp file
};

#endif // DECODER_MAD_H_INCLUDED
