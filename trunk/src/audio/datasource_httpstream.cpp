#include "datasource_httpstream.h"

#include "../error-handling.h"
#include "../util/StrFormat.h"

#include <sstream>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

using namespace std;

#define MAX_RETRIES 16

HTTPStreamDataSource::HTTPStreamDataSource() {
	conn = NULL;
	exhaustion_counter=0;
}

void HTTPStreamDataSource::disconnect() {
	if(conn) {
		conn->disconnect();
		delete conn;
		conn = NULL;
	}
}

HTTPStreamDataSource::~HTTPStreamDataSource() {
	disconnect();
}

void HTTPStreamDataSource::do_http_get_request(string host, string path) {
	stringstream ss;
	if(path.size() == 0) path = "/";
	if(path[0] != '/') path = "/" + path;
	ss << "GET " << path << " HTTP/1.1\r\n";
	ss << "Host: " << host << "\r\n";
	// NOTE: There is something funny going on with these user-agent strings.
	//       It appears as though some(?) shoutcast(?) servers do some sort of
	//       checking on these, and deny access for some reason.
	//
	// NOTE2: Also it seems some shoutcast servers do some checking of the
	//        user-agent string to see if we're a webbrowser or a music
	//        player and return different results depending on that.
	//        So let's pretend we're foobar for now.
	//ss << "User-Agent: Mozilla\r\n";     // Works (really generic...)
	ss << "User-Agent: foobar/1.0\r\n";  // Works (ripped from foobar2000)
	//ss << "User-Agent: sfdsfwef\r\n";    // Works (random keyboard bashing)
	//ss << "User-Agent: MPMPd\r\n";         // Works (this is us)	
	//ss << "User-Agent: mpmpd\r\n";       // Does not work... WTF????
	ss << "Accept: */*\r\n";
	// we can't handle metadata yet, so don't ask for it
	ss << "Icy-MetaData: 0\r\n";
	ss << "Connection: close\r\n";
	ss << "\r\n";

	std::string sendmsg = ss.str();
	if (sendmsg.size() != conn->send((uint8*)sendmsg.c_str(), sendmsg.size()))
		throw HTTPException("Unexpected error while sending HTTP request");
}


#define INSERT_NON_EMPTY(map, index, item) { \
	string s = item; \
	if(!s.empty()) \
		result[index] = s;\
}

/**
 * This function (modeled somewhat after PHP::parse_url()) parses an URL and 
 * returns an associative array containing any of the various components of
 * the URL that are present.
 * This function is not meant to validate the given URL, it only breaks it up.
 * Partial URLs are also accepted, parse_url() tries its best to parse them
 * correctly. 
 * TODO: query    - after the question mark (?)
 *       fragment - after the hashmark (#) 
 */
map<string, string> HTTPStreamDataSource::parse_url(string str) {
	map<string, string> result;
	if(str.empty()) return result;

	string::size_type begin, end;
	begin = str.find("://");
	if(begin != string::npos) { // scheme is present
		INSERT_NON_EMPTY(result, "scheme", str.substr(0, begin));
		begin += 3; // at least '://' follows begin
	}
	else result["scheme"] = "http";

	if(begin == string::npos) begin = 0;

	end = str.find_first_of('/', begin); // str[begin] .. str[end] is the user:pass@host:port part
	if(end == string::npos) {
		end = str.size();
		str = str + '/';
	}
	string::size_type at_pos   = str.find_first_of('@', begin);

	// see if there is an '@'-sign between 'scheme://' and the first '/'
	if(at_pos != string::npos && at_pos < end) { 
		string::size_type colon_pos = str.find_first_of(':', begin);
		if(colon_pos != string::npos && colon_pos < at_pos) {
			INSERT_NON_EMPTY(result, "user", str.substr(begin, colon_pos - begin));
			INSERT_NON_EMPTY(result, "password", str.substr(colon_pos + 1, at_pos - colon_pos - 1));
		}
		else {
			INSERT_NON_EMPTY(result, "user", str.substr(begin, at_pos - begin));
		}
		begin = at_pos + 1; // at least 1 '/' character follows at_pos
	}

	// str[begin] .. str[end] is now the host:port part
	string::size_type colon_pos = str.find_first_of(':', begin);
	if(colon_pos != string::npos && colon_pos < end) {
		INSERT_NON_EMPTY(result, "host", str.substr(begin, colon_pos - begin));
		INSERT_NON_EMPTY(result, "port", str.substr(colon_pos+1, end - colon_pos - 1));
	}
	else {
		INSERT_NON_EMPTY(result, "host", str.substr(begin, end - begin));
	}
	string path = str.substr(end);
	if(path.empty())
		path = "/";
	result["path"] = path;

	return result;
}

std::vector<std::string> HTTPStreamDataSource::receive_http_headers() {
	std::vector<std::string> result;
	std::string current_line;
	int received_bytes = 0;
	int rn_count = 0; // number of \r\n (CRLF, newlines) seen
	do {
		char c;
		received_bytes++;
		if((conn->receive((uint8*)&c, 1) != 1) || (received_bytes > 8*1024)) // we try to receive at most 8k of header data
			throw HTTPException(STRFORMAT("Error while receiving HTTP headers after %i bytes", received_bytes));
		current_line += c;
		if(c == '\r')  rn_count += 1;
		else if(c == '\n') {
			rn_count += 1;
			if(rn_count == 2) {
				result.push_back(current_line);
				current_line.clear();
			}
		}
		else rn_count = 0;
	} while (rn_count != 4);
	return result;
}

void HTTPStreamDataSource::http_connect(const string& host, uint16 port) {
	disconnect();
	ipv4_addr address = ipv4_lookup(host);
	conn = new tcp_socket(address, port); // fixme: try{} ?
}

HTTPStreamDataSource::HTTPStreamDataSource(string str)
{
	conn = NULL;
	exhaustion_counter=0;

	bool connection_established = false;
	while(!connection_established) {
		// Parse str into parts
		map<string, string> url_parts = parse_url(str);

		if(url_parts.find("host") == url_parts.end())
			throw HTTPException(STRFORMAT("The URL `%s' is not valid (can not find a hostname in this)", str));

		uint16 port = 80;
		if(url_parts.find("port") != url_parts.end()) {
			try {
				port = boost::lexical_cast<uint16>(url_parts["port"]);
			}
			catch(boost::bad_lexical_cast e) {}
		}
		url_parts["port"] = boost::lexical_cast<string>(port);

		// Connect
		http_connect(url_parts["host"], port);
		do_http_get_request(url_parts["host"] + ":" + url_parts["port"], url_parts["path"]);
		std::vector<string> headers = receive_http_headers();
		if(headers.size() > 0) {
			// NOTE: Eventhough we asked not to send ICY metadata, we may still
			//       get an ICY response header instead of HTTP. This is ok though
			//       as this should not contain the ICY-MetaInt parameter (and thus
			//       not send any metadata).
			if(headers[0].find("HTTP") == 0 || headers[0].find("ICY") == 0) {
				string::size_type begin = headers[0].find_first_of(' ');
				string::size_type end = headers[0].find_first_of(' ', begin + 1);
				string ss = headers[0].substr(begin + 1, end - begin - 1);
				int result = boost::lexical_cast<int>(ss);
				switch(result) {
					case 200: { // HTTP/1.1 200 OK
						connection_established = true;
					}; break;
					case 301:   // HTTP/1.0 301 Moved Permanently
					case 302:   // HTTP/1.0 302 Found 
					case 303:   // HTTP/1.1 303 See Other
					case 307: { // HTTP/1.1 307 Temporary Redirect
						bool found_new_location = false;
						BOOST_FOREACH(string& s, headers) {
							if(s.find("Location: ") == 0) {
								str = s.substr(10, s.size() - 10 - 2); // -10 for 'Location ', -2 for '\r\n'
								found_new_location = true;
							}
						}
						if(!found_new_location)
							throw HTTPException("Could not find redirect location");
					}; break;
					default:
						throw HTTPException("Unprocessed HTTP status code: HTTP " + ss);
				}
			}
		}
		else throw HTTPException("Did not receive any HTTP headers");
	}

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

uint32 HTTPStreamDataSource::getpos() {
	return datarpos;
}

bool HTTPStreamDataSource::exhausted() {
	return (exhaustion_counter == MAX_RETRIES);
}
