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
};

extern std::vector<boost::function<IDecoder* (IDataSource*)> > decoderlist;

#endif//DECODER_INTERFACE_H
