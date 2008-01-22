#include "decoder_vorbis.h"
#include "../error-handling.h"

VorbisDecoder::VorbisDecoder() : IDecoder() {
	packet = NULL;
	dsp_state = NULL;
	block = NULL;
	info = NULL;
	comment = NULL;
	reset();
}

void VorbisDecoder::reset() {
	if(packet) {
		delete packet->packet;
		delete packet;
	}
	packet = new ogg_packet();
	packet->packet = new unsigned char[1<<16]; //Maximum page size = 64k?
	packet->bytes = 0;
	packet->b_o_s = 0;
	packet->e_o_s = 0;
	packet->granulepos=0;
	packet->packetno=0;

	if(dsp_state) delete dsp_state;
	dsp_state = new vorbis_dsp_state();

	if(block) delete block;
	block = new vorbis_block();

	if(info) delete info;
	info = new vorbis_info();

	if(comment) delete comment;
	comment = new vorbis_comment();
}


VorbisDecoder::~VorbisDecoder() {

}

uint32 VorbisDecoder::doDecode(uint8* buf, uint32 max, uint32 req) {
	dcerr("");
}

IDecoder* VorbisDecoder::tryDecode(IDataSource* ds) {
	dcerr("");
	reset();
	ds->reset();

	packet->bytes = ds->read(packet->packet, 1<<16);
	if(!packet->bytes) return NULL;

	vorbis_info_init(info);
	vorbis_comment_init(comment);

	if(vorbis_synthesis_headerin(info,comment,packet)<0){
		return NULL;
	}
		/* error case; not a vorbis header * /
		fprintf(stderr,"This Ogg bitstream does not contain Vorbis "
			"audio data.\n");
		exit(1);
	}
/*
	if(ogg_stream_pagein(&os,&og)<0){
		/* error; stream version mismatch perhaps * /
		fprintf(stderr,"Error reading first page of Ogg bitstream data.\n");
		exit(1);
	}

	if(ogg_stream_packetout(&os,&op)!=1){
		/* no page? must not be vorbis * /
		fprintf(stderr,"Error reading initial header packet.\n");
		exit(1);
	}

	if(vorbis_synthesis_headerin(&vi,&vc,&op)<0){
		/* error case; not a vorbis header * /
		fprintf(stderr,"This Ogg bitstream does not contain Vorbis "
			"audio data.\n");
		exit(1);
	}*/
	return NULL;
}
