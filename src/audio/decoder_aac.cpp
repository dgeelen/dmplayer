#include "decoder_aac.h"
#include "../error-handling.h"
#include <algorithm>

#include <boost/cstdint.hpp>

using namespace std;

/* Thank you mplayer :-) */
int AACDecoder::aac_probe() {
	int pos = -1;
	for(uint i = 0; i < buffer_fill - 4; ++i ) {
		if( ((buffer[i] == 0xff) && ((buffer[i+1] & 0xf6) == 0xf0)) || (buffer[i] == 'A' && buffer[i+1] == 'D' && buffer[i+2] == 'I' && buffer[i+3] == 'F')) {
			pos = i;
			break;
		}
	}
	return pos;
}

static void printCaps(long caps) {
	#define CAN_DECODE(mask) dcerr("faad2 can" << ((caps&mask)?" ":" NOT ") << "decode " << #mask);
	CAN_DECODE(LC_DEC_CAP);
	CAN_DECODE(MAIN_DEC_CAP);
	CAN_DECODE(LTP_DEC_CAP);
	CAN_DECODE(LD_DEC_CAP);
	CAN_DECODE(ERROR_RESILIENCE_CAP);
	CAN_DECODE(FIXED_POINT_CAP);
}

AACDecoder::AACDecoder() : IDecoder(AudioFormat()) {
	initialize();
	printCaps(decoder_capabilities);
}

void AACDecoder::fill_buffer() {
	while(buffer_fill != BLOCK_SIZE) {
		buffer_fill += datasource->getData( buffer + buffer_fill, BLOCK_SIZE - buffer_fill);
	}
}

AACDecoder::AACDecoder(IDataSourceRef ds) : IDecoder(AudioFormat()) {
	this->datasource = ds;
	initialize();
	datasource->reset();
	fill_buffer();
	int pos = aac_probe();
	if(pos>0) {
		memmove(buffer, buffer+pos, BLOCK_SIZE - pos );
		buffer_fill -= pos;
		fill_buffer();
	}
	if(pos<0) throw "This does not appear to be an AAC stream";

	boost::uint32_t sample_rate;
	boost::uint8_t channels;
	long bytes_used = faacDecInit(decoder_handle, buffer, BLOCK_SIZE, &sample_rate, &channels);
	if(bytes_used<0) {
		dcerr("Error while initializing libfaad2");
		throw "Error while initializing libfaad2";
	}
	if(bytes_used) {
		memmove(buffer, buffer + bytes_used, BLOCK_SIZE - bytes_used);
		buffer_fill -= bytes_used;
		fill_buffer();
	}
	audioformat.SampleRate = sample_rate;
	audioformat.Channels = channels;
	audioformat.BitsPerSample=16;
	audioformat.LittleEndian=true;
	audioformat.SignedSample=true; //FIXME: ???
}

void AACDecoder::initialize() {
	buffer_fill = 0;
	sample_buffer_size=0;
	sample_buffer_index=0;
	sample_buffer = NULL;
	decoder_capabilities = faacDecGetCapabilities();
	decoder_handle = faacDecOpen();
	decoder_config = faacDecGetCurrentConfiguration(decoder_handle);
	/* Adjust configuration */
	decoder_config->defSampleRate    = 44100;
	decoder_config->outputFormat     = FAAD_FMT_16BIT;
	decoder_config->useOldADTSFormat = 0;
	decoder_config->downMatrix       = 1;
	/* then call SetConfiguration */
	int error = faacDecSetConfiguration(decoder_handle, decoder_config);
	if(error==0) {
		dcerr("Error: invalid configuration");
		throw "Error: invalid configuration";
	}
}

AACDecoder::~AACDecoder() {
}

IDecoderRef AACDecoder::tryDecode(IDataSourceRef ds) {
	try {
		return IDecoderRef(new AACDecoder(ds));
	}
	catch(...) {
		return IDecoderRef();
	}
}

uint32 AACDecoder::getData(uint8* buf, uint32 len) {
	const int bytes_per_sample = audioformat.BitsPerSample/8;
	uint32 bytes_todo = len;
	uint32 bytes_done = 0;
	uint8* ptr = buf;

	if(sample_buffer_size - sample_buffer_index) {
		uint32 to_copy = min(sample_buffer_size - sample_buffer_index, len);
		memcpy(ptr, sample_buffer + sample_buffer_index, to_copy);
		sample_buffer_index    += to_copy;
		bytes_done             += to_copy;
		bytes_todo             -= to_copy;
		ptr                    += to_copy;
	}
	bool done = (bytes_done == len); //Only when sample_buffer has been emptied
	int error_count=0;

	while(!done) {
		faacDecFrameInfo frame_info;
		sample_buffer = (uint8*) faacDecDecode(decoder_handle, &frame_info, buffer, BLOCK_SIZE);
		memmove(buffer, buffer + frame_info.bytesconsumed, BLOCK_SIZE - frame_info.bytesconsumed);
		buffer_fill-=frame_info.bytesconsumed;
		fill_buffer();
		/* Now we have new samples */
		if (frame_info.error == 0) {
			sample_buffer_size = frame_info.samples*bytes_per_sample;
			uint32 to_copy = min(sample_buffer_size, bytes_todo);
			memcpy(ptr, sample_buffer, to_copy);
			ptr+=to_copy;
			sample_buffer_index = to_copy;
			bytes_todo -= to_copy;
			bytes_done += to_copy;
			done = (bytes_done == len);
		} else {
			dcerr(datasource->getpos() << " : " << faacDecGetErrorMessage(frame_info.error));
			if(error_count < 10) {
				++error_count;
				memmove(buffer, buffer + 1, BLOCK_SIZE - 1);
				buffer_fill-=1;
				fill_buffer();
				int pos = aac_probe();
				if(pos) {
					memmove(buffer, buffer + pos, BLOCK_SIZE - pos);
					buffer_fill-=pos;
					fill_buffer();
					dcerr("faad2 decode error, resync'd after "<<pos<<" bytes");
				}
			}
			else {
				return bytes_done;
			}
		}
	}
	return bytes_done;
}
