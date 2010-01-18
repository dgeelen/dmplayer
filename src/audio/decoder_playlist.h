#ifndef DECODER_PLAYLIST_H
#define DECODER_PLAYLIST_H

#include "decoder_interface.h"
#include "datasource_interface.h"

class PlaylistDecoder : public IDecoder {
	private:
		static const size_t BLOCK_SIZE = 1024;
		uint8 buffer[BLOCK_SIZE];
		size_t bufpos;
		size_t bufsize;
		IDataSourceRef source;
		IDecoderRef decoder;

		void fill_buffer();
		void parse_buffer();
	public:
		PlaylistDecoder(IDataSourceRef source);
		~PlaylistDecoder();

		static IDecoderRef tryDecode(IDataSourceRef datasource);
		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
};

#endif//DECODER_RAW_H
