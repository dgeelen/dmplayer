#ifndef DECODER_MENCODER_H
#define DECODER_MENCODER_H

#include "decoder_interface.h"


typedef boost::shared_ptr<class MencoderDecoderImplementation> MencoderDecoderImplRef;

typedef boost::shared_ptr<class MencoderDecoder> MencoderDecoderRef;
class MencoderDecoder : public IDecoder {
	private:
		MencoderDecoderImplRef impl;
	public:
		static IDecoderRef tryDecode(IDataSourceRef ds);

		MencoderDecoder(IDataSourceRef ds);

		virtual uint32 getData(uint8* buf, uint32 max);
};

#endif DECODER_MENCODER_H