#include "decoder_vorbis.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

VorbisDecoder::VorbisDecoder() : IDecoder(AudioFormat()) {
	datasource = NULL;
	initialize();
}

VorbisDecoder::VorbisDecoder(IDataSource* ds) : IDecoder(AudioFormat()) {
	datasource = ds;
	initialize();
	datasource->reset();
	/* Initialize libvorbis */
	if(!read_vorbis_headers()) throw "VorbisDecoder: Error while reading vorbis header";
	/* FIXME: These are only used when we have a data source, and unitialized after playing a full track */
	vorbis_synthesis_init(dsp_state, info);
	vorbis_block_init(dsp_state, block);
}

void VorbisDecoder::initialize() {
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
	packet->granulepos=-1;
	packet->packetno=-1;
}

void VorbisDecoder::uninitialize() {
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
	uninitialize();
	if(datasource) datasource->reset();
	initialize();
}

VorbisDecoder::~VorbisDecoder() {
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
	/* Throw the comments plus a few lines about the bitstream we're decoding */
	cout << "------< Vorbis Stream Info >------\n"
	     << "  Vorbis version " << info->version << "\n"
	     << "  " << info->channels << " channels" << "\n"
	     << "  " << info->rate << "Hz" << "\n"
	     << "  bitrate: " << info->bitrate_lower << " < " << info->bitrate_nominal << " < " << info->bitrate_upper << "\n"
	     << "  bitrate window size " << info->bitrate_window << "\n"
	     << "------< Vorbis Comments    >------\n";
	char **ptr=comment->user_comments;
	while(*ptr){
		cout << *ptr << "\n";
		++ptr;
	}
	cout << "Encoded by: " << comment->vendor << "\n"
	     << "------< Vorbis Stream Info >------\n";
	return true;
}

uint32 VorbisDecoder::doDecode(uint8* buffer, uint32 max, uint32 req) {
	const int bytes_per_sample = info->channels * sizeof(ogg_int16_t);
	const int samples_requested = req / bytes_per_sample;
	if(max < req) throw "VorbisDecoder: Logic error!";
	uint32 samples_decoded = 0;
	bool done = false;
	while(!done) {
		float **pcm;
		int samples=vorbis_synthesis_pcmout(dsp_state, &pcm);
		if(samples>0) {
			int samples_todo = min(samples_requested - samples_decoded, (uint32)samples);
			bool clipflag=0;
			for(int channel=0; channel<info->channels; ++channel) { /* convert floats to 16 bit signed ints (host order) and interleave */
				ogg_int16_t *ptr = ((ogg_int16_t*)buffer) + (samples_decoded*info->channels) + channel;
				float *mono_channel=pcm[channel];
				for(int i=0; i<samples_todo; ++i) {
					#if 1
					int val=mono_channel[i]*32767.f;
					#else /* optional dither */
					int val=mono_channel[i]*32767.f+drand48()-0.5f;
					#endif
					/* might as well guard against clipping */
					if(val>32767) {
						val=32767;
						clipflag=true;
					}
					if(val<-32768) {
						val=-32768;
						clipflag=true;
					}
					*ptr=val;
					ptr+=info->channels;
			  }
			}
			samples_decoded+=samples_todo;
			if(samples_decoded==samples_requested) done = true;
			if(clipflag) dcerr("Clipping in frame " << (long)(dsp_state->sequence));
			vorbis_synthesis_read(dsp_state, samples_todo); /* tell libvorbis how many samples we actually consumed */
		}
		else {
			construct_next_packet();
			if(!packet->bytes) done=true;
			if(vorbis_synthesis(block, packet)==0) {
				vorbis_synthesis_blockin(dsp_state, block); //FIXME: Else?
			}
			else dcerr("Warning: could not vorbis_synthesis_blockin()");
		}
	}
	return samples_decoded*bytes_per_sample;
}

IDecoder* VorbisDecoder::tryDecode(IDataSource* ds) {
	datasource=ds;
	reset();

	if(read_vorbis_headers())
	return new VorbisDecoder(ds);
	return NULL;
}
