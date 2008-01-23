#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"
#include "audiosource_interface.h"

#include "../types.h"

#include <vector>
#include <boost/function.hpp>

class IDecoder: public IAudioSource {
	public:
		IDecoder(AudioFormat af) : IAudioSource(af) {};
		virtual IDecoder* tryDecode(IDataSource*) = 0;

		static IDecoder* findDecoder(IDataSource* ds);
};

#endif//DECODER_INTERFACE_H
