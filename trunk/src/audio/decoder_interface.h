#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"
#include "../types.h"

class IDecoder {
	private:
		int buffersize;
	public:
		IDecoder() {};
		virtual ~IDecoder() {};
		virtual IDecoder* tryDecode(IDataSource*) = 0;
		void SetBufferSize(int bufsize) {buffersize = bufsize;};
		int GetBufferSize() {return buffersize;};

		virtual uint32 doDecode(char* buf, uint32 max, uint32 req = 0) = 0;
};

#endif//DECODER_INTERFACE_H
