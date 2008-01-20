#include "decoder_vorbis.h"
#include "../error-handling.h"

VorbisDecoder::VorbisDecoder() : IDecoder() {

}

VorbisDecoder::~VorbisDecoder() {

}

IDecoder* VorbisDecoder::tryDecode(IDataSource* ds) {
	dcerr("");
	return NULL;
}
