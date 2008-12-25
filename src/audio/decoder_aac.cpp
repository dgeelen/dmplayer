#include "decoder_aac.h"
#include "../error-handling.h"
#include <algorithm>

using namespace std;

/* Thank you mplayer :-) */
int AACDecoder::aac_probe() {
	int pos = -1;
	for(uint i = 0; i + 4 < buffer_fill; ++i ) {
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

void AACDecoder::fill_buffer() {
	while(buffer_fill != BLOCK_SIZE) {
		int read = datasource->getData( buffer + buffer_fill, BLOCK_SIZE - buffer_fill);
		if (read == 0 && datasource->exhausted()) break;
		buffer_fill += read;
	}
}

bool AACDecoder::exhausted() {
	return datasource->exhausted() && (buffer_fill==0);
}

AACDecoder::AACDecoder(IDataSourceRef ds) : IDecoder(AudioFormat()) {
	dcerr("new AACDecoder");
	this->datasource = ds;
	initialize();
	datasource->reset();
	fill_buffer();
	int pos = aac_probe();
	if(pos>0) {
		memmove(buffer, buffer+pos, buffer_fill - pos );
		buffer_fill -= pos;
	}
	if (pos<0)
		throw Exception("This does not appear to be an AAC stream");

	FAAD_UINT32TYPE sample_rate = 0;
	uint8 channels = 0;
	fill_buffer();
	long bytes_used = faacDecInit(decoder_handle, buffer, buffer_fill, &sample_rate, &channels);
	if ( (bytes_used<0) || (sample_rate == 0) || (channels == 0))
		throw Exception("Error while initializing AAC stream");
	if(bytes_used) {
		memmove(buffer, buffer + bytes_used, BLOCK_SIZE - bytes_used);
		buffer_fill -= bytes_used;
	}

	faacDecFrameInfo frame_info;
	fill_buffer();
	sample_buffer = (uint8*) faacDecDecode(decoder_handle, &frame_info, buffer, buffer_fill);
	memmove(buffer, buffer + frame_info.bytesconsumed, BLOCK_SIZE - frame_info.bytesconsumed);
	buffer_fill-=frame_info.bytesconsumed;

	if (frame_info.error != 0)
		throw Exception("Error decoding first AAC frame");

	const int bytes_per_sample = audioformat.BitsPerSample/8;
	sample_buffer_size = frame_info.samples*bytes_per_sample;

	audioformat.SampleRate = sample_rate;
	audioformat.Channels = channels;
	audioformat.BitsPerSample=16;
	audioformat.LittleEndian=true;
	audioformat.SignedSample=true; //FIXME: ???
}

void AACDecoder::initialize() {
	decoder_handle = faacDecOpen();
	if (!decoder_handle) throw Exception("Failed to initialize libfaad library");
	onDestroy.add(boost::bind(&faacDecClose, decoder_handle));
	buffer_fill = 0;
	sample_buffer_size=0;
	sample_buffer_index=0;
	sample_buffer = NULL;
	decoder_capabilities = faacDecGetCapabilities();
	decoder_config = faacDecGetCurrentConfiguration(decoder_handle);
	/* Adjust configuration */
	decoder_config->defSampleRate    = 44100;
	decoder_config->outputFormat     = FAAD_FMT_16BIT;
	decoder_config->useOldADTSFormat = 0;
	decoder_config->downMatrix       = 1;
	/* then call SetConfiguration */
	int error = faacDecSetConfiguration(decoder_handle, decoder_config);
	if (error == 0)
		throw Exception("Error: invalid configuration");
}

AACDecoder::~AACDecoder() {
	dcerr("Shut down");
}

IDecoderRef AACDecoder::tryDecode(IDataSourceRef ds) {
	try {
		return IDecoderRef(new AACDecoder(ds));
	}
	catch (Exception& e) {
		dcerr("AAC tryDecode failed: " << e.what());
		return IDecoderRef();
	}
}

uint32 AACDecoder::getData(uint8* buf, uint32 len) {
	const int bytes_per_sample = audioformat.BitsPerSample/8;
	uint32 bytes_todo = len;
	uint32 bytes_done = 0;
	uint8* ptr = buf;

	if(sample_buffer_size - sample_buffer_index > 0) {
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
		fill_buffer();
		if (buffer_fill == 0) return bytes_done;
		sample_buffer = (uint8*) faacDecDecode(decoder_handle, &frame_info, buffer, buffer_fill);
		memmove(buffer, buffer + frame_info.bytesconsumed, BLOCK_SIZE - frame_info.bytesconsumed);
		buffer_fill-=frame_info.bytesconsumed;
		if((frame_info.error==0) && (frame_info.samples>0)) { /* Now we have new samples */
			sample_buffer_size = frame_info.samples*bytes_per_sample;
			uint32 to_copy = min(sample_buffer_size, bytes_todo);
			memcpy(ptr, sample_buffer, to_copy);
			ptr+=to_copy;
			sample_buffer_index = to_copy;
			bytes_todo -= to_copy;
			bytes_done += to_copy;
			done = (bytes_done == len);
		}
		else if(frame_info.error) { /* Either some error occured, or we just need more data */
			if(error_count < 10) {
				/* This dcerr is for debugging of weird streams only */
				dcerr("Offending bytes: " << hex
				                          << (int)buffer[ 0] << " "
				                          << (int)buffer[ 1] << " "
				                          << (int)buffer[ 2] << " "
				                          << (int)buffer[ 3] << " "
				                          << (int)buffer[ 4] << " "
				                          << (int)buffer[ 5] << " "
				                          << (int)buffer[ 6] << " "
				                          << (int)buffer[ 7] << " "
				                          << (int)buffer[ 8] << " "
				                          << (int)buffer[ 9] << " "
				                          << (int)buffer[10] << " "
				                          << (int)buffer[11] << " "
				                          << (int)buffer[12] << " "
				                          << (int)buffer[13] << " "
				                          << (int)buffer[14] << " "
				                          << (int)buffer[15] << " "
				                          << (int)buffer[16] << " "
				);

				++error_count;
				if (buffer_fill > 0 && frame_info.bytesconsumed == 0) {
					memmove(buffer, buffer + 1, buffer_fill - 1); //fixme?
					buffer_fill-=1;
				}
				fill_buffer();
				int pos = aac_probe();
				if(pos>=0) {
					memmove(buffer, buffer + pos, buffer_fill - pos);
					buffer_fill-=pos;
					dcerr("At " << datasource->getpos() << " bytes: " << faacDecGetErrorMessage(frame_info.error) << ", resynced after "<<pos<<" bytes");
				} else {
					buffer_fill=0;
				}
			}
			else { /* Too many errors */
				return bytes_done;
			}
		}
	}
	return bytes_done;
}
