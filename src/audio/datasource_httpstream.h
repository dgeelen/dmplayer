#ifndef DECODER_HTTPSTREAM_H
#define DECODER_HTTPSTREAM_H

#include "decoder_interface.h"
#include "../network-core.h"

#include <string>

#define HTTP_STREAM_BUFFER_SIZE 1024*64
#define HTTP_STREAM_RECV_CHUNK 1024*8

class HTTPStreamDataSource : public IDataSource {
	private:
		uint8 data[HTTP_STREAM_BUFFER_SIZE];
		uint datarpos;
		uint datawpos;
		uint datalen;
		uint dataofs;
		tcp_socket* conn;
	public:
		HTTPStreamDataSource();
		HTTPStreamDataSource(std::string url);
		~HTTPStreamDataSource();

		virtual long getpos() { return 0; };
		virtual bool exhausted() { return false; };
		virtual void reset();

		virtual uint32 read(uint8* buffer, uint32 len);
};

#endif//DECODER_OGGVORBISFILE_H
