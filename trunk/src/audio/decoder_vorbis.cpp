#include "decoder_vorbis.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

VorbisDecoder::VorbisDecoder(IDataSourceRef ds) : IDecoder(AudioFormat()) {
	datasource = ds;
	datasource->reset();

	vorbis_info_init(&info);
	onDestroy.add(boost::bind(&vorbis_info_clear, &info));

	vorbis_comment_init(&comment);
	onDestroy.add(boost::bind(&vorbis_comment_clear, &comment));

	packet.packet = packetbuffer;
	packet.bytes = 0;
	packet.b_o_s = 2;
	packet.e_o_s = 0;
	packet.granulepos=-1;
	packet.packetno=-1;

	if (!read_vorbis_headers())
		throw Exception("VorbisDecoder: Error while reading vorbis header");

	audioformat.LittleEndian = true;
	audioformat.Channels = info.channels;
	audioformat.BitsPerSample = 16;
	audioformat.SampleRate = info.rate;
	audioformat.SignedSample = true;

	vorbis_synthesis_init(&dsp_state, &info);
	onDestroy.add(boost::bind(&vorbis_dsp_clear, &dsp_state));

	vorbis_block_init(&dsp_state, &block);
	onDestroy.add(boost::bind(&vorbis_block_clear, &block));
}

VorbisDecoder::~VorbisDecoder() {
}

bool VorbisDecoder::exhausted() {
	return datasource->exhausted() && packet.e_o_s;
}

void VorbisDecoder::construct_next_packet() {
	packet.bytes = datasource->getData(packet.packet, 1<<16);
	packet.b_o_s = max(--(packet.b_o_s), (long int)0);
	packet.e_o_s = (packet.bytes==0) ? 1 : 0;
	packet.granulepos++;
	packet.packetno++;
}

bool VorbisDecoder::read_vorbis_headers() {
	for(int i=0; i<3; ++i) {
		construct_next_packet();
		//FIXME: Find out what vorbis_synthesis_headerin returns
		int result = vorbis_synthesis_headerin(&info, &comment, &packet);
		if(result<0) {
			dcerr("vorbis_synthesis_headerin error #" << result << " in header packet "<<i);
			return false;
		}
	}
	/* Throw the comments plus a few lines about the bitstream we're decoding */
	cout << "------< Vorbis Stream Info >------\n"
	     << "  Vorbis version " << info.version << "\n"
	     << "  " << info.channels << " channels" << "\n"
	     << "  " << info.rate << "Hz" << "\n"
	     << "  bitrate: " << info.bitrate_lower << " < " << info.bitrate_nominal << " < " << info.bitrate_upper << "\n"
	     << "  bitrate window size " << info.bitrate_window << "\n"
	     << "------< Vorbis Comments    >------\n";
	char **ptr=comment.user_comments;
	while(*ptr){
		cout << *ptr << "\n";
		++ptr;
	}
	cout << "Encoded by: " << comment.vendor << "\n"
	     << "------< Vorbis Stream Info >------\n";
	return true;
}

uint32 VorbisDecoder::getData(uint8* buffer, uint32 len)
{
	const int bytes_per_sample = info.channels * sizeof(ogg_int16_t);
	const int samples_requested = len / bytes_per_sample;
	uint32 samples_decoded = 0;
	bool done = false;
	while(!done) {
		float **pcm;
		int samples=vorbis_synthesis_pcmout(&dsp_state, &pcm);
		if(samples>0) {
			int samples_todo = min(samples_requested - samples_decoded, (uint32)samples);
			bool clipflag=0;
			for(int channel=0; channel<info.channels; ++channel) { /* convert floats to 16 bit signed ints (host order) and interleave */
				ogg_int16_t *ptr = ((ogg_int16_t*)buffer) + (samples_decoded*info.channels) + channel;
				float *mono_channel=pcm[channel];
				for(int i=0; i<samples_todo; ++i) {
					#if 1
					int val=(int)(mono_channel[i]*32767.f);
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
					ptr+=info.channels;
			  }
			}
			samples_decoded+=samples_todo;
			if(samples_decoded==samples_requested) done = true;
			if(clipflag) dcerr("Clipping in frame " << (long)(dsp_state.sequence));
			vorbis_synthesis_read(&dsp_state, samples_todo); /* tell libvorbis how many samples we actually consumed */
		}
		else {
			if(packet.e_o_s) done=true;
			construct_next_packet();
			if(vorbis_synthesis(&block, &packet)==0) {
				vorbis_synthesis_blockin(&dsp_state, &block); //FIXME: Else?
			}
// 			else dcerr("Warning: could not vorbis_synthesis_blockin()"); //EOF?
		}
	}
	return samples_decoded*bytes_per_sample;
}

IDecoderRef VorbisDecoder::tryDecode(IDataSourceRef ds) {
	IDecoderRef decoder;
	try {
		decoder = IDecoderRef(new VorbisDecoder(ds));
	} catch (Exception &e) {
		VAR_UNUSED(e); // in debug mode
		dcerr("VorbisDecoder::tryDecode failed: " << e.what());
	}
	return decoder;
}
