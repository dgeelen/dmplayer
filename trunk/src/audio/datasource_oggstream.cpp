#include "datasource_oggstream.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

OGGStreamDataSource::OGGStreamDataSource( OGGDecoderRef decoder, long stream_id ) {
	this->decoderw = decoder;
	this->stream_id = stream_id;
	packet = NULL;
	is_exhausted = false;
	total_bytes_read=0;
	bytes_leftover = 0;
}

OGGStreamDataSource::~OGGStreamDataSource() {
	if(packet) delete packet;
};

void OGGStreamDataSource::reset() {
	throw Exception("Impossible OGGStreamDataSource::reset()");
}

long OGGStreamDataSource::getpos() {
	return total_bytes_read;
}

bool OGGStreamDataSource::exhausted() {
	return is_exhausted;
}

/**
 * Returns at most 1 packet of data per call
 * If less data is requested than contained in the packet, subsequent
 * calls will return the remainder until the full packet has been returned
 * @param buffer Where to store packet data
 * @param len Size of buffer
 * @return buffer is filled with as much data as is contained in 1 packet
 */
uint32 OGGStreamDataSource::getData(uint8* const buffer, uint32 len) {
	OGGDecoderRef decoder(decoderw);
	if (!decoder) throw Exception("Weak reference invalid!");
	if(!is_exhausted) {
		if(bytes_leftover) {
			uint32 todo = min(bytes_leftover, len);
			memcpy(buffer, packet->packet + packet->bytes - bytes_leftover, todo);
			bytes_leftover-=todo;
			total_bytes_read+=todo;
			return todo;
		}
		delete packet;
		packet = decoder->get_packet_from_stream(stream_id);
		if(packet) {
			int n = min((unsigned long)packet->bytes, len);
			memcpy(buffer, packet->packet, n);
			total_bytes_read+=n;
			if((uint)packet->bytes > len) {
				bytes_leftover = packet->bytes-n;
				dcerr("Warning: Splitting packet, "<<bytes_leftover<<" bytes leftover");
			}
			else {
				delete packet;
				packet = NULL;
			}
			return n;
		}
		is_exhausted = true;
	}
	return 0;
}
