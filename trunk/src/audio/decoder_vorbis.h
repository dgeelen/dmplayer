#ifndef DECODER_VORBIS_H
#define DECODER_VORBIS_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <vorbis/codec.h>

class VorbisDecoder : public IDecoder {
	public:
		VorbisDecoder();
		~VorbisDecoder();
		IDecoder* tryDecode(IDataSource* ds);
		uint32 doDecode(char* buf, uint32 max, uint32 req) { return 0; }; //FIXME: implement in .cpp file
	private:
};

#endif//DECODER_LIBOGG_H
