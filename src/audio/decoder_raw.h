#ifndef DECODER_RAW_H
#define DECODER_RAW_H

#include "decoder_interface.h"
#include "datasource_interface.h"

class RawDecoder : public IDecoder{
	private:
		IDataSource* source;
	public:
		RawDecoder();
		RawDecoder(IDataSource* source);
		~RawDecoder();
		IDecoder* tryDecode(IDataSource* datasource);
		uint32 doDecode(unsigned char* const buf, uint32 max, uint32 req);
};

#endif//DECODER_RAW_H
