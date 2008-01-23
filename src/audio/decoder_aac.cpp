#include "decoder_aac.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

/* Thank you mplayer :-) */
int AACDecoder::aac_probe() {
	int pos = -1;
	for(int i = 0; i < buffer_fill - 4; ++i ) {
		if( ((buffer[i] == 0xff) && ((buffer[i+1] & 0xf6) == 0xf0)) || (buffer[i] == 'A' && buffer[i+1] == 'D' && buffer[i+2] == 'I' && buffer[i+3] == 'F')) {
			pos = i;
			break;
		}
	}
	return pos;
}

AACDecoder::AACDecoder() : IDecoder(AudioFormat()) {
	initialize();
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
		memcpy(buffer, buffer+pos, BLOCK_SIZE - pos );
		buffer_fill -= pos;
		fill_buffer();
	}
	if(pos<0) throw "This does not appear to be an AAC stream";

	uint32 sample_rate;
	uint8 channels;
	long bytes_used = faacDecInit(decoder_handle, buffer, BLOCK_SIZE, &sample_rate, &channels);
	if(bytes_used<0) {
		dcerr("Error while initializing libfaad2");
		throw "Error while initializing libfaad2";
	}
	if(bytes_used) {
		memcpy(buffer, buffer + bytes_used, BLOCK_SIZE - bytes_used);
		buffer_fill -= bytes_used;
		fill_buffer();
	}
	audioformat.SampleRate = sample_rate;
	audioformat.Channels = channels;
	audioformat.BitsPerSample=16;
	audioformat.LittleEndian=true;
	audioformat.SignedSample=true; //FIXME: ???
}


static void printCaps(long caps) {
	#define CAN_DECODE(mask) dcerr("faad2 can " << ((caps&mask)?" ":"NOT ") << "decode " << #mask);
	CAN_DECODE(LC_DEC_CAP);
	CAN_DECODE(MAIN_DEC_CAP);
	CAN_DECODE(LTP_DEC_CAP);
	CAN_DECODE(LD_DEC_CAP);
	CAN_DECODE(ERROR_RESILIENCE_CAP);
	CAN_DECODE(FIXED_POINT_CAP);
}

void AACDecoder::initialize() {
	buffer_fill = 0;
	sample_buffer_size=0;
	sample_buffer_index=0;
	sample_buffer = NULL;
	//decoder_capabilities = faacDecGetCapabilities();
// 	printCaps(decoder_capabilities);
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

static char *aac_error_message[] = {
	"No error",
	"Gain control not yet implemented",
	"Pulse coding not allowed in short blocks",
	"Invalid huffman codebook",
	"Scalefactor out of range",
	"Unable to find ADTS syncword",
	"Channel coupling not yet implemented",
	"Channel configuration not allowed in error resilient frame",
	"Bit error in error resilient scalefactor decoding",
	"Error decoding huffman scalefactor (bitstream error)",
	"Error decoding huffman codeword (bitstream error)",
	"Non existent huffman codebook number found",
	"Invalid number of channels",
	"Maximum number of bitstream elements exceeded",
	"Input data buffer too small",
	"Array index out of range",
	"Maximum number of scalefactor bands exceeded",
	"Quantised value out of range",
	"LTP lag out of range",
	"Invalid SBR parameter decoded",
	"SBR called without being initialised",
	"Unexpected channel configuration change",
	"Error in program_config_element",
	"First SBR frame is not the same as first AAC frame",
	"Unexpected fill element with SBR data",
	"Not all elements were provided with SBR data",
	"LTP decoding not available",
	"Output data buffer too small",
	"CRC error in DRM data",
	"PNS not allowed in DRM data stream",
	"No standard extension payload allowed in DRM",
	"PCE shall be the first element in a frame",
	"Bitstream value not allowed by specification"
};

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
	}
	bool done = (bytes_done == len); //Only when sample_buffer has been emptied
	int error_count=0;
	while(!done) {
		faacDecFrameInfo frame_info;
		sample_buffer = (uint8*) faacDecDecode(decoder_handle, &frame_info, buffer, len);
		memcpy(buffer, buffer + frame_info.bytesconsumed, BLOCK_SIZE - frame_info.bytesconsumed);
		buffer_fill-=frame_info.bytesconsumed;
		fill_buffer();
		/* Now we have new samples */
		if((frame_info.error==0) && (frame_info.samples>0)) {
			sample_buffer_size = frame_info.samples*bytes_per_sample;
			uint32 to_copy = min(sample_buffer_size, bytes_todo);
			memcpy(ptr, sample_buffer, to_copy);
			ptr+=to_copy;
			sample_buffer_index = to_copy;
			bytes_todo -= to_copy;
			bytes_done += to_copy;
			done = (bytes_done == len);
		}
		else {
			if(error_count < 10) {
				++error_count;
				memcpy(buffer, buffer + 1, BLOCK_SIZE - 1);
				buffer_fill-=1;
				fill_buffer();
				int pos = aac_probe();
				if(pos) {
					memcpy(buffer, buffer + pos, BLOCK_SIZE - pos);
					buffer_fill-=pos;
					fill_buffer();
					dcerr("faad2 decode error, resync'd after "<<pos<<" bytes");
				}
			}
			else {
				dcerr(aac_error_message[frame_info.error]);
				throw aac_error_message[frame_info.error];
			}
		}
	}
	return bytes_done;
}
