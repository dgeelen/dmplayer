#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"

class IDecoder {
	private:
		int buffersize;
	public:
		IDecoder() {};
		virtual ~IDecoder() {};
		virtual IDecoder* tryDecode(IDataSource*) = 0;
		void SetBufferSize(int bufsize) {buffersize = bufsize;};
		int GetBufferSize() {return buffersize;};
};

#endif//DECODER_INTERFACE_H
