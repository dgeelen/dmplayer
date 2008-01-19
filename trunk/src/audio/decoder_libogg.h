#ifndef DECODER_LIBOGG_H
#define DECODER_LIBOGG_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <ogg.h>

class OGGDecoder : public IDecoder {
	public:
		OGGDecoder();
		~OGGDecoder();
		IDecoder* tryDecode(IDataSource* ds);
		uint32 doDecode(char* buf, uint32 max, uint32 req) { return 0; }; //FIXME: implement in .cpp file
	private:
		ogg_sync_state* sync;
		ogg_page* page;
		char* buffer;
};

#endif//DECODER_LIBOGG_H
