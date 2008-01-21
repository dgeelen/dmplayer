#include "datasource_oggstream.h"
#include "../error-handling.h"

OGGStreamDataSource::OGGStreamDataSource( OGGDecoder* decoder, long stream_id ) {
	this->decoder = decoder;
	this->stream_id = stream_id;
	reset();
}

OGGStreamDataSource::~OGGStreamDataSource() {
	is_exhausted = false;
};

void OGGStreamDataSource::reset() {
	this->decoder->reset();
}

long OGGStreamDataSource::getpos() {
	return 0;
}

bool OGGStreamDataSource::exhausted() {
	return is_exhausted;
}

unsigned long OGGStreamDataSource::read(char* const buffer, unsigned long len) {
	dcerr("do NOT use int OGGStreamDataSource.read(char*, int), use ogg_packet* OGGStreamDataSource.read() instead");
	is_exhausted = true;

/*
	// Simply to test dumping the output of the OGGDecoder (seems to work)
	long to_read = len;
	long n=0;
	while(to_read>0) {
		ogg_packet* packet = read();
		if(packet) {
		unsigned long l = std::min(to_read, packet->bytes);
		memcpy(buffer+n, packet->packet, l);
		to_read-=packet->bytes;
		n+=packet->bytes;
		}
		else to_read=0;
	}
	fwrite(buffer, 1, n, dump);
	return n;
*/
	return 0;
}

ogg_packet* OGGStreamDataSource::read() {
	return decoder->get_packet_from_stream(stream_id);
}
