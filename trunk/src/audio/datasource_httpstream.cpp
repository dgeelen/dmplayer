#include "datasource_httpstream.h"

//hax
#include "decoder_oggvorbisfile.h"

#include <sstream>

using namespace std;

HTTPStreamDataSource::HTTPStreamDataSource()
{
}

HTTPStreamDataSource::~HTTPStreamDataSource()
{
}

HTTPStreamDataSource::HTTPStreamDataSource(std::string url)
{
	if (strnicmp(url.c_str(), "http://", 7) != 0) throw "not a valid url";

	string hoststr = url.substr(7);
	int colonpos = hoststr.find_first_of(':');
	int slashpos = hoststr.find_first_of('/');
	string portstr;
	string substr;
	if (slashpos == string::npos) {
		portstr = hoststr.substr(colonpos+1);
		substr = "";
	} else {
		portstr = hoststr.substr(colonpos+1, slashpos-colonpos-1);
		substr = hoststr.substr(slashpos+1);
		hoststr.erase(slashpos);
	}
	uint16 port;
	if (colonpos == string::npos)
		port = 80;
	else {
		port = atoi(portstr.c_str());
		hoststr.erase(colonpos);
	}

	ipv4_addr address;
	{
		unsigned long hostaddr = inet_addr( hoststr.c_str() );
		if( hostaddr == INADDR_NONE )
		{
			hostent *host = gethostbyname( hoststr.c_str() );
			hostaddr = *reinterpret_cast<unsigned long *>( host->h_addr_list[0] );

			if( hostaddr == INADDR_NONE )
			{
				throw "failed to find host adress";
			}
		}
		address.full = hostaddr;
	}

	conn = new tcp_socket(address, port);

	string sendmsg;
	{
		stringstream ss;
		ss << "GET /" << substr << " HTTP/1.0\r\n";
		ss << "Host: " << hoststr << ":" << port << "\r\n";
		ss << "User-Agent: mpmpd-vunknown\r\n";
		ss << "Icy-MetaData: 1\r\n";
		ss << "Connection: close\r\n";
		ss << "\r\n";
		sendmsg = ss.str();
	}

	if (sendmsg.size() != conn->send((uint8*)sendmsg.c_str(), sendmsg.size())) {
		throw "error sending http request";
	}

	char headers[1025];
	int hpos = 0;
	int hend = 0;
	do {
		if (hpos == 1024) throw "error receiving shoutcast headers";
		if (conn->receive((uint8*)headers+hpos, 1) != 1) throw "error receiving shoutcast headers";
		if (headers[hpos] == '\r') ++hend;
		else if (headers[hpos] == '\n') ++hend;
		else hend = 0;
		++hpos;
	} while (hend != 4);
	headers[hpos] = 0;
	datarpos = 0;
	datawpos = 0;
	datalen = 0;
	dataofs = 0;
}

unsigned long HTTPStreamDataSource::read(uint8* buffer, uint32 len)
{
	uint torecv = HTTP_STREAM_BUFFER_SIZE-1;
	torecv = min(torecv, HTTP_STREAM_BUFFER_SIZE-datawpos);
	torecv = min(torecv, HTTP_STREAM_BUFFER_SIZE-(datalen+1));
	torecv = min(torecv, HTTP_STREAM_RECV_CHUNK);
	torecv = min(torecv, len);
	if (torecv) {
		uint arecv = conn->receive(data+datawpos, torecv);
		datalen += arecv;
		datawpos += arecv;
		if (datawpos == HTTP_STREAM_BUFFER_SIZE) {
			datawpos = 0;
			dataofs += HTTP_STREAM_BUFFER_SIZE;
		}
	}

	uint toread = HTTP_STREAM_BUFFER_SIZE-1;
	toread = min(toread, HTTP_STREAM_BUFFER_SIZE-datarpos);
	toread = min(toread, datalen);
	memcpy(buffer, data+datarpos, toread);
	datarpos += toread;
	datalen -= toread;
	if (datarpos == HTTP_STREAM_BUFFER_SIZE)
		datarpos = 0;
	return toread;
}

void HTTPStreamDataSource::reset()
{
	if (dataofs != 0) throw "error!";
	datalen += datarpos;
	datarpos = 0;
}
