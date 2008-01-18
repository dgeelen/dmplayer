#ifndef BACKEND_LIBAO_H
#define BACKEND_LIBAO_H
#include <ao.h>
#include "backend_interface.h"

class LibAOBackend : public IBackend {
	public:
		LibAOBackend(IDecoder* dec);
		virtual ~LibAOBackend();
	private:
		ao_device *device;
		ao_sample_format format;
};

#endif//BACKEND_LIBAO_H
