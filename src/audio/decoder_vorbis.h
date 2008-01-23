#ifndef DECODER_VORBIS_H
#define DECODER_VORBIS_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <vorbis/codec.h>

class VorbisDecoder : public IDecoder {
	public:
		VorbisDecoder();
		VorbisDecoder(IDataSourceRef ds);
		~VorbisDecoder();
		void reset();
		IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 max);
	private:
		IDataSourceRef datasource;
		void initialize();
		void uninitialize();
		void construct_next_packet();
		bool read_vorbis_headers();
		vorbis_dsp_state* dsp_state;
		vorbis_block* block;
		vorbis_info* info;
		vorbis_comment* comment;

		/* Reconstructing packets */
		ogg_packet* packet;
};

#endif//DECODER_LIBOGG_H
