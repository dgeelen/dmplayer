#ifndef DECODER_VORBIS_H
#define DECODER_VORBIS_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <vorbis/codec.h>

class VorbisDecoder : public IDecoder {
	public:
		VorbisDecoder();
		~VorbisDecoder();
		void reset();
		IDecoder* tryDecode(IDataSource* ds);
		uint32 doDecode(uint8* buf, uint32 max, uint32 req) { return 0; }; //FIXME: implement in .cpp file
	private:
		vorbis_dsp_state* dsp_state;
		vorbis_block* block;
		vorbis_info* info;
		vorbis_comment* comment;

		/* Reconstructing packets */
		ogg_packet* packet;
};

#endif//DECODER_LIBOGG_H
