#ifndef BACKEND_INTERFACE_H
#define BACKEND_INTERFACE_H

#include "decoder_interface.h"

class IBackend {
private:
		IDecoder* source;
	public:
		IBackend(IDecoder* i) : source(i) {};
		virtual ~IBackend() {};
};

#endif//BACKEND_INTERFACE_H