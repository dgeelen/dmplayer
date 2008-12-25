#include "decoder_flac.h"

#include "../error-handling.h"

FlacDecoder::FlacDecoder(IDataSourceRef ds_)
	: IDecoder(AudioFormat())
	, ds(ds_)
{
	try_decode = true;
	FLAC__StreamDecoderInitStatus stat = init();

	bufptr = NULL;

	process_single();

	if (bufptr == NULL) throw Exception("Invalid flac stream");

	try_decode = false;
	eos = false;

}

bool FlacDecoder::exhausted() {
	return ds->exhausted() && eos;
}

void FlacDecoder::metadata_callback (const ::FLAC__StreamMetadata *metadata) {
	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		audioformat.Channels = metadata->data.stream_info.channels;
		audioformat.BitsPerSample =  metadata->data.stream_info.bits_per_sample;
		audioformat.SampleRate = metadata->data.stream_info.sample_rate;
		audioformat.LittleEndian = true;
		audioformat.SignedSample = true;

		bufsize = audioformat.Channels * (audioformat.BitsPerSample/8) * metadata->data.stream_info.max_blocksize;
		bufref = boost::shared_array<uint8>(new uint8[bufsize]);
		bufptr = bufref.get();
		bufpos = 0;
		buflen = 0;
	}
}

IDecoderRef FlacDecoder::tryDecode(IDataSourceRef ds) {
	try {
		IDecoderRef ret(new FlacDecoder(ds));
		return ret;
	}
	catch (Exception& e) {
		VAR_UNUSED(e); // in debug mode
		dcerr("Flac tryDecode failed: " << e.what());
		return IDecoderRef();
	}
}

/* IDecoder Interface */
uint32 FlacDecoder::getData(uint8* buf, uint32 len)
{
	uint32 bufdone = 0;
	do {
		if (bufpos < buflen && bufdone < len) {
			uint32 dlen = std::min(len - bufdone, buflen - bufpos);
			memcpy(buf + bufdone, bufptr + bufpos, dlen);
			bufdone += dlen;
			bufpos += dlen;
		}

		if (bufpos == buflen) {
			bufpos = 0;
			buflen = 0;
			this->process_single();
		}
	} while (bufdone < len && bufpos < buflen);

	eos = bufdone==0;
	return bufdone;
}

FlacDecoder::~FlacDecoder() {
	dcerr("Shut down");
}

/* FLAC::Decoder::Stream Interface (callbacks) */
FLAC__StreamDecoderReadStatus FlacDecoder::read_callback(FLAC__byte buf[],size_t * len)
{
	*len = ds->getData(buf, *len);
	if(((*len==0) && ds->exhausted()) || (try_decode && (ds->getpos()>8*1024))) {
		bufptr = NULL;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus FlacDecoder::write_callback(const FLAC__Frame * header,const FLAC__int32 *const data[])
{
	for (uint i = 0; i < header->header.blocksize; ++i) {
		for (int j = 0; j < audioformat.Channels; ++j) {
			for (int k = 0; k*8 < audioformat.BitsPerSample; ++k) {
				if (buflen < bufsize)
					bufptr[buflen++] = ((char*)&data[j][i])[k];
				else
					dcerr("flac decode buffer full, samples dropped");
			}
		}
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacDecoder::error_callback(FLAC__StreamDecoderErrorStatus err)
{
	dcerr("flac: error:" << err);
	if(err) bufptr = NULL;
}
