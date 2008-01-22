#include "decoder_vorbis.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

VorbisDecoder::VorbisDecoder() : IDecoder() {
	dcerr("");
	datasource = NULL;
	reset();
}

VorbisDecoder::VorbisDecoder(IDataSource* ds) : IDecoder() {
	dcerr(ds);
	datasource = ds;
	reset();
	/* Initialize libvorbis */
	if(!read_vorbis_headers()) throw "VorbisDecoder: Error while reading vorbis header";
}

void VorbisDecoder::initialize() {
	dcerr("");
	dsp_state = new vorbis_dsp_state();
	block = new vorbis_block();
	info = new vorbis_info();
	comment = new vorbis_comment();
	vorbis_info_init(info);
	vorbis_comment_init(comment);

	packet = new ogg_packet();
	packet->packet = new unsigned char[1<<16]; //Maximum page size = 64k?
	packet->bytes = 0;
	packet->b_o_s = 2;
	packet->e_o_s = 0;
	packet->granulepos=0;
	packet->packetno=0;
}

void VorbisDecoder::uninitialize() {
	dcerr("");
	if(packet) {
		delete packet->packet;
		packet->packet = NULL;
		delete packet;
		packet=NULL;
	}
	if(dsp_state) {
		delete dsp_state;
		dsp_state = NULL;
	}
	if(block) {
		delete block;
		block = NULL;
	}
	if(info) {
		vorbis_info_clear(info);
		delete info;
		info = NULL;
	}
	if(comment) {
		vorbis_comment_clear(comment);
		delete comment;
		comment = NULL;
	}
}

void VorbisDecoder::reset() {
	dcerr("");
	uninitialize();
	if(datasource) datasource->reset();
	initialize();
}

VorbisDecoder::~VorbisDecoder() {
	dcerr("");
	uninitialize();
}

void VorbisDecoder::construct_next_packet() {
	packet->bytes = datasource->read(packet->packet, 1<<16);
	packet->b_o_s = max(--(packet->b_o_s), (long int)0);
	packet->e_o_s = datasource->exhausted();
	packet->granulepos++;
	packet->packetno++;
}

bool VorbisDecoder::read_vorbis_headers() {
	for(int i=0; i<3; ++i) {
		construct_next_packet();
		//FIXME: Find out what vorbis_synthesis_headerin returns
		int result = vorbis_synthesis_headerin(info, comment, packet);
		if(result<0) {
			dcerr("vorbis_synthesis_headerin error #" << result << " in header packet "<<i);
			return false;
		}
	}
	return true;
}

uint32 VorbisDecoder::doDecode(uint8* buf, uint32 max, uint32 req) {
	dcerr("");
}

IDecoder* VorbisDecoder::tryDecode(IDataSource* ds) {
	dcerr("");
	datasource=ds;
	reset();

	if(read_vorbis_headers())
	return new VorbisDecoder(ds);
	return NULL;
}
