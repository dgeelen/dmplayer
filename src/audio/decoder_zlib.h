#ifndef DECODER_ZLIB_H
#define DECODER_ZLIB_H

#include "decoder_interface.h"
#include "datasource_interface.h"

class ZLibDecoder {
	public:
		static IDecoderRef tryDecode(IDataSourceRef datasource);
};

#endif//DECODER_ZLIB_H
