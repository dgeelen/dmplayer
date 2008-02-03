#ifndef DECODER_HTTPSTREAM_H
#define DECODER_HTTPSTREAM_H

#include "decoder_interface.h"
#include "../network-core.h"

#include <string>

#define HTTP_STREAM_BUFFER_SIZE ( (uint32) 1024*64*2 )
#define HTTP_STREAM_RECV_CHUNK  ( (uint32) 1024*32 )

class HTTPStreamDataSource : public IDataSource {
	private:
		uint8 data[HTTP_STREAM_BUFFER_SIZE]; // data buffer
		uint32 datarpos;    // read position in buffer
		uint32 datawpos;    // write position in buffer
		uint32 datalen;     // amount of valid data in buffer
		uint32 dataofs;     // current offset to begin of stream of buffer
		tcp_socket* conn;
		int exhaustion_counter;
	public:
		HTTPStreamDataSource();
		HTTPStreamDataSource(std::string url);
		~HTTPStreamDataSource();

		long getpos();
		bool exhausted();
		virtual void reset();

		virtual uint32 getData(uint8* buffer, uint32 len);
};

#endif//DECODER_OGGVORBISFILE_H
