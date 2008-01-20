#include "datasource_oggstream.h"
#include "../error-handling.h"
OGGStreamDataSource::OGGStreamDataSource( OGGDecoder* decoder, long stream_id ) {
	dcerr("");
	this->decoder = decoder;
	this->stream_id = stream_id;
}

OGGStreamDataSource::~OGGStreamDataSource() {
	dcerr("");
	is_exhausted = false;
};

void OGGStreamDataSource::reset() {
	dcerr("");
	this->decoder->reset();
}

long OGGStreamDataSource::getpos() {
	return 0;
}

bool OGGStreamDataSource::exhausted() {
	dcerr("");
	return is_exhausted;
}

int OGGStreamDataSource::read(char* const buffer, int len) {
	dcerr("");
	dcerr("do NOT use int OGGStreamDataSource.read(char*, int), use ogg_packet* OGGStreamDataSource.read() instead");
	is_exhausted = true;
	return 0;
}

ogg_packet* OGGStreamDataSource::read() {
	dcerr("");
	return decoder->get_packet_from_stream(stream_id);
}
