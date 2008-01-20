#include "datasource_oggstream.h"

OGGStreamDataSource::OGGStreamDataSource( OGGDecoder* decoder, long stream_id ) {
	this->decoder = decoder;
	this->stream_id = stream_id;
}

OGGStreamDataSource::~OGGStreamDataSource() {
};

void OGGStreamDataSource::reset() {
	this->decoder->reset();
}

bool OGGStreamDataSource::exhausted() {
}

ogg_packet* OGGStreamDataSource::read() {
	return decoder->get_packet_from_stream(stream_id);
}
