#ifndef DECODER_OGGVORBISFILE_H
#define DECODER_OGGVORBISFILE_H

#include "decoder_interface.h"

#include <vorbis/vorbisfile.h>

class OGGVorbisFileDecoder : public IDecoder{
	private:
		OggVorbis_File* oggFile;
		OGGVorbisFileDecoder(OggVorbis_File* oggFile);
		int bitStream;
	public:
		OGGVorbisFileDecoder();
		~OGGVorbisFileDecoder();
		IDecoder* tryDecode(IDataSource* datasource);
		uint32 doDecode(char* buf, uint32 max, uint32 req);
};

#endif//DECODER_OGGVORBISFILE_H
