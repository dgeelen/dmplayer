#include "datasource_httpstream.h"

#include "../error-handling.h"

#include <sstream>
#include <algorithm>

using namespace std;

#define MAX_RETRIES 16

HTTPStreamDataSource::HTTPStreamDataSource()
{
	conn = NULL;
	exhaustion_counter=0;
}

HTTPStreamDataSource::~HTTPStreamDataSource()
{
	if (conn) {
		conn->disconnect();
		conn = NULL;
	}
}

HTTPStreamDataSource::HTTPStreamDataSource(std::string url)
{
	exhaustion_counter=0;
	#ifdef __linux__
	if (strncmp(url.c_str(), "http://", 7) != 0) throw HTTPException("not a valid url");
	#else
	if (_strnicmp(url.c_str(), "http://", 7) != 0) throw HTTPException("not a valid url");
	#endif
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
				throw HTTPException("failed to find host adress");
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
		// we can't handle metadata yet, so don't ask for it
		//ss << "Icy-MetaData: 1\r\n";
		ss << "Connection: close\r\n";
		ss << "\r\n";
		sendmsg = ss.str();
	}

	if (sendmsg.size() != conn->send((uint8*)sendmsg.c_str(), sendmsg.size())) {
		throw HTTPException("error sending http request");
	}

	char headers[1025];
	int hpos = 0;
	int hend = 0;
	do {
		if (hpos == 1024) throw HTTPException("error receiving shoutcast headers");
		if (conn->receive((uint8*)headers+hpos, 1) != 1) throw HTTPException("error receiving shoutcast headers");
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

unsigned long HTTPStreamDataSource::getData(uint8* buffer, uint32 len)
{
	if((len>datalen) || (dataofs>0)) {
		uint32 torecv = HTTP_STREAM_BUFFER_SIZE-1;
		torecv = min(torecv, HTTP_STREAM_BUFFER_SIZE-datawpos);
		torecv = min(torecv, HTTP_STREAM_BUFFER_SIZE-(datalen+1));
		torecv = min(torecv, HTTP_STREAM_RECV_CHUNK);
		torecv = min(torecv, len);

		if (torecv) {
			uint arecv = conn->receive(data+datawpos, torecv);
			//cout << "REVC = req : " << len << "  try: " <<
			datalen += arecv;
			datawpos += arecv;
			if (datawpos == HTTP_STREAM_BUFFER_SIZE) {
				datawpos = 0;
				dataofs += 1;
			}
			exhaustion_counter = min((arecv==0) ? ++exhaustion_counter : 0, MAX_RETRIES);
		}
	}
	uint32 toread = HTTP_STREAM_BUFFER_SIZE-1;
	toread = min(toread, HTTP_STREAM_BUFFER_SIZE-datarpos);
	toread = min(toread, datalen);
	toread = min(toread, len);

	memcpy(buffer, data+datarpos, toread);
	datarpos += toread;
	datalen -= toread;
	if (datarpos == HTTP_STREAM_BUFFER_SIZE)
		datarpos = 0;
	return toread;
}

void HTTPStreamDataSource::reset()
{
	if (dataofs != 0) throw HTTPException("error, reset() after too many reads");
	datalen += datarpos;
	datarpos = 0;
}

long HTTPStreamDataSource::getpos() {
	return datarpos;
}

bool HTTPStreamDataSource::exhausted() {
	return (exhaustion_counter == MAX_RETRIES);
}
