#ifndef DECODER_OGGVORBISFILE_H
#define DECODER_OGGVORBISFILE_H

#include "decoder_interface.h"

#include <vorbis/vorbisfile.h>

class OGGVorbisFileDecoder : public IDecoder{
	private:
		OggVorbis_File* oggFile;
		OGGVorbisFileDecoder(AudioFormat af, OggVorbis_File* oggFile);
		int bitStream;
	public:
		OGGVorbisFileDecoder();
		~OGGVorbisFileDecoder();
		IDecoder* tryDecode(IDataSource* datasource);
		uint32 getData(uint8* buf, uint32 max);
};

#endif//DECODER_OGGVORBISFILE_H
