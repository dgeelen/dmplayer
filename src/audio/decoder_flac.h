#ifndef DECODER_FLAC_H
#define DECODER_FLAC_H

#include "decoder_interface.h"
#include <FLAC++/all.h>

#include <boost/shared_array.hpp>

typedef boost::shared_ptr<class FlacDecoder> FlacDecoderRef;
class FlacDecoder: public IDecoder, public FLAC::Decoder::Stream {
	private:
		uint8* bufptr;
		boost::shared_array<uint8> bufref;
		uint32 bufpos;
		uint32 buflen;
		uint32 bufsize;
		IDataSourceRef ds;
		bool eos;
		bool try_decode;
	public:
		static IDecoderRef tryDecode(IDataSourceRef ds_);

		FlacDecoder(IDataSourceRef ds);
		bool exhausted();

		/* IDecoder Interface */
		uint32 getData(uint8* buf, uint32 len);

		/* FLAC::Decoder::Stream Interface (callbacks) */
		FLAC__StreamDecoderReadStatus read_callback(FLAC__byte [],size_t *);
		FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame *,const FLAC__int32 *const []);
		void error_callback(FLAC__StreamDecoderErrorStatus);
		bool set_metadata_respond_all () {return true;};
		virtual void 	metadata_callback (const ::FLAC__StreamMetadata *metadata);
};

#endif//DECODER_FLAC_H
