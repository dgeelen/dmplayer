#ifndef DECODER_HTTPSTREAM_H
#define DECODER_HTTPSTREAM_H

#include "decoder_interface.h"
#include "../network-core.h"

#include <string>

#define HTTP_STREAM_BUFFER_SIZE 1024*64
#define HTTP_STREAM_RECV_CHUNK 1024*32

class HTTPStreamDataSource : public IDataSource {
	private:
		uint8 data[HTTP_STREAM_BUFFER_SIZE]; // data buffer
		uint datarpos;    // read position in buffer
		uint datawpos;    // write position in buffer
		uint datalen;     // amount of valid data in buffer
		uint dataofs;     // current offset to begin of stream of buffer
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
