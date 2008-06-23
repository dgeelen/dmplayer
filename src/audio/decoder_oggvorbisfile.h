#ifndef DECODER_OGGVORBISFILE_H
#define DECODER_OGGVORBISFILE_H

#include "decoder_interface.h"

#include <vorbis/vorbisfile.h>

class OGGVorbisFileDecoder : public IDecoder{
	private:
		IDataSourceRef datasource;
		OggVorbis_File* oggFile;
		OGGVorbisFileDecoder(AudioFormat af, IDataSourceRef ds, OggVorbis_File* oggFile);
		int bitStream;
		bool eos;
	public:
		~OGGVorbisFileDecoder();
		static IDecoderRef tryDecode(IDataSourceRef datasource);
		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
};

#endif//DECODER_OGGVORBISFILE_H
