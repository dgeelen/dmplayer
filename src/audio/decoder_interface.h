#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"
#include "audiosource_interface.h"

#include "../types.h"

#include <vector>

typedef boost::shared_ptr<class IDecoder> IDecoderRef;
class IDecoder: public IAudioSource {
	public:
		IDecoder(AudioFormat af) : IAudioSource(af) {};
		virtual IDecoderRef tryDecode(IDataSourceRef) = 0;

		static IDecoderRef findDecoder(IDataSourceRef ds);
};

#endif//DECODER_INTERFACE_H
