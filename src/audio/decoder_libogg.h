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
	private:
		ogg_sync_state* sync;
		ogg_page* page;
		char* buffer;
};

#endif//DECODER_LIBOGG_H
