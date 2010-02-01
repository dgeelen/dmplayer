#ifndef DECODER_HTTPSTREAM_H
#define DECODER_HTTPSTREAM_H

#include "decoder_interface.h"
#include "../network/network-core.h"

#include <string>
#include <map>

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

		void http_connect(const std::string& str, uint16 port);
		void do_http_get_request(std::string host, std::string path);
		std::vector<std::string> receive_http_headers();
		static std::map<std::string, std::string> parse_url(std::string str);
		void disconnect();
	public:
		HTTPStreamDataSource();
		HTTPStreamDataSource(std::string url);
		~HTTPStreamDataSource();

		uint32 getpos();
		bool exhausted();
		virtual void reset();

		virtual uint32 getData(uint8* buffer, uint32 len);
};

#endif//DECODER_OGGVORBISFILE_H
