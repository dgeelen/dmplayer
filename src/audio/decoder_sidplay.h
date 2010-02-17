#ifndef DECODER_SIDPLAY_H
#define DECODER_SIDPLAY_H

#include "decoder_interface.h"
#include <boost/shared_ptr.hpp>
#include <sidplay/sidplay2.h>

class SidplayDecoder: public IDecoder {
	public:
		~SidplayDecoder();
		SidplayDecoder(IDataSourceRef source);

		/* IDecoder Interface */
		bool exhausted();
		static IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 len);

	private:
		IDataSourceRef datasource;
		sidplay2 player;
		boost::shared_ptr<SidTune> tune;
		boost::shared_ptr<sidbuilder> sidemu;
};

#endif //DECODER_SIDPLAY_H
