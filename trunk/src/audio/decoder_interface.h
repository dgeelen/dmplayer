#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"

class IDecoder {
	public:
		IDecoder() {};
		virtual ~IDecoder() {};

		virtual IDecoder* tryDecode(IDataSource*) = 0;
};

#endif//DECODER_INTERFACE_H