

#include "datasource_oggstream.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

OGGStreamDataSource::OGGStreamDataSource( OGGDecoderRef decoder, long stream_id ) {
	this->decoder = decoder;
	this->stream_id = stream_id;
	is_exhausted = false;
	total_bytes_read=0;
	reset();
}

OGGStreamDataSource::~OGGStreamDataSource() {
	total_bytes_read=0;
	is_exhausted = false;
};

void OGGStreamDataSource::reset() {
	is_exhausted = false;
	total_bytes_read=0;
	this->decoder->reset();
}

long OGGStreamDataSource::getpos() {
	return total_bytes_read;
}

bool OGGStreamDataSource::exhausted() {
	return is_exhausted;
}

/**
 * Returns at most 1 packet of data per call
 * @param buffer Where to store packet data
 * @param len Size of buffer
 * @return buffer is filled with as much data as is contained in 1 packet
 */
uint32 OGGStreamDataSource::getData(uint8* const buffer, uint32 len) {
	ogg_packet* packet = decoder->get_packet_from_stream(stream_id);
	if(packet) {
		if(packet->bytes > (int)len) dcerr("Warning: packet won't fit buffer!");
		int n = min((unsigned long)packet->bytes, len);
		memcpy(buffer, packet->packet, n);
		delete packet;
		total_bytes_read+=n;
		return n;
	}
	is_exhausted = true;
	return 0;
}

ogg_packet* OGGStreamDataSource::read() {
	return decoder->get_packet_from_stream(stream_id);
}
