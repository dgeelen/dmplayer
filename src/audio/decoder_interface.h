#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"
#include "audiosource_interface.h"

#include "../types.h"

#include <vector>

typedef boost::shared_ptr<class IDecoder> IDecoderRef;
class IDecoder: public IAudioSource {
	public:
		IDecoder(AudioFormat af = AudioFormat()) : IAudioSource(af) {};

		static IDecoderRef findDecoder(IDataSourceRef ds);

		//subclasses need to implement one of these methods:
		//IDecoder tryDecode(IDataSourceRef);
		//static IDecoder tryDecodeStatic(IDataSourceRef);

};

#endif//DECODER_INTERFACE_H
