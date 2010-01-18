#include "decoder_playlist.h"
#include "../error-handling.h"
#include "datasource_httpstream.h"
#include <string>

PlaylistDecoder::PlaylistDecoder(IDataSourceRef ds)
	: IDecoder(AudioFormat())
	, source(ds)
{
	bufpos = bufsize = 0;
	ds->reset();

	parse_buffer();
	if (!decoder) throw IOException("no streams found in playlist");

	audioformat = decoder->getAudioFormat();
}

void PlaylistDecoder::fill_buffer() {
	if (bufpos > 0) {
		bufsize -= bufpos;
		memmove(buffer, buffer+bufpos, bufsize);
		bufpos = 0;
	}

	while (bufsize != BLOCK_SIZE) {
		int read = source->getData(buffer + bufsize, BLOCK_SIZE - bufsize);
		if (read == 0 && source->exhausted()) break;
		bufsize += read;
	}
}

void PlaylistDecoder::parse_buffer()
{
	decoder.reset();
	while (true) {
		int pos = -1;
		fill_buffer();
		for (size_t i = 0; i < bufsize; ++i) {
			if (buffer[i] == '\n' || buffer[i] == '\r') {
				pos = i;
				break;
			}
		}
		if (pos == -1) return;
		std::string line((char*)buffer, (char*)buffer+pos);
		bufpos += pos+1;

		if (line.empty() || line[0] == '#') continue;

		size_t strpos;
		std::string url;
		if (line.size() > 4 && line.substr(0,4) == "File" && (strpos = line.find_first_of("=")) != std::string::npos) {
			url = line.substr(strpos+1);
		} else
			url = line;

		try {
			IDataSourceRef httpstream(new HTTPStreamDataSource(url));
			decoder = IDecoder::findDecoder(httpstream);
		} catch (...) {
			continue;
		}

		if (decoder) return;
	}
}

PlaylistDecoder::~PlaylistDecoder()
{
	dcerr("Shut down");
}

bool PlaylistDecoder::exhausted()
{
	do {
		bool done = true;
		if (decoder) done = decoder->exhausted();
		if (done) {
			parse_buffer();
			if (!decoder)
				return true;
			if (audioformat != decoder->getAudioFormat())
				return true;
		} else
			return false;
	} while (true);
}

IDecoderRef PlaylistDecoder::tryDecode(IDataSourceRef datasource)
{
	IDecoderRef ret;
	try {
		ret = IDecoderRef(new PlaylistDecoder(datasource));
	} catch (...) {
		ret.reset();
	}
	return ret;
}

uint32 PlaylistDecoder::getData(uint8* buf, uint32 len)
{
	uint32 res = 0;
	do {
		uint32 read = 0;
		try {
			if (decoder) read = decoder->getData(buf+res, len-res);
		} catch (...) {
			read = 0;
		}
		if (read == 0) {
			parse_buffer();
			if (!decoder)
				return res;
			if (audioformat != decoder->getAudioFormat())
				return res;
		}
		res += read;
	} while (res < len);
	return res;
}
