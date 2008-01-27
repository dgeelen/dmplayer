#ifndef DECODER_VORBIS_H
#define DECODER_VORBIS_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <vorbis/codec.h>
#include "../util/ScopeExitSignal.h"

class VorbisDecoder : public IDecoder {
	public:
		~VorbisDecoder();
		static IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 max);
	private:
		VorbisDecoder(IDataSourceRef ds);
		IDataSourceRef datasource;
		void construct_next_packet();
		bool read_vorbis_headers();
		vorbis_dsp_state dsp_state;
		vorbis_block block;
		vorbis_info info;
		vorbis_comment comment;

		/* Reconstructing packets */
		ogg_packet packet;
		uint8 packetbuffer[1<<16]; //Maximum page size = 64k?

		ScopeExitSignal onDestroy; // should be last data member
};

#endif//DECODER_LIBOGG_H
