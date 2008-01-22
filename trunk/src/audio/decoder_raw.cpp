#include "decoder_raw.h"
#include <string>

RawDecoder::RawDecoder() : IDecoder(Audio_Format())
{
	source = NULL;
}

RawDecoder::RawDecoder(Audio_Format af, IDataSource* source) : IDecoder(af)
{
	this->source = source;
	//tryDecode(source);
}

RawDecoder::~RawDecoder()
{
}

IDecoder* RawDecoder::tryDecode(IDataSource* datasource)
{
	datasource->reset();
	uint8 hdr[48];
	uint8* uhdr = (uint8*)hdr;
	if (datasource->read(hdr, 48) != 48) return NULL;
#define STRCHECK(x, str) for (unsigned int i = 0; i < strlen(str); ++i) if (hdr[x+i] != str[i]) return NULL;

	STRCHECK( 0, "RIFF");
	STRCHECK( 8, "WAVE");
	STRCHECK(12, "fmt ");
	STRCHECK(36, "data");

	uint16 fmt      = uhdr[20] + (uhdr[21] << 8);
	uint16 channels = uhdr[22] + (uhdr[23] << 8);
	uint32 srate    = uhdr[24] + (uhdr[25] << 8) + (uhdr[26] << 16) + (uhdr[27] << 24);
	uint16 bits     = uhdr[34] + (uhdr[35] << 8);

	if (fmt != 1)       return NULL;
	if (channels != 2)  return NULL; // TODO: mono->stereo filter?
	//if (srate != 22050) return NULL; // TODO: samplerate conversion filter?
	if (bits != 16)     return NULL; // TODO: bps conversion filter?
	// or, return a decoder with the info from the header, so the controller
	// can change the backend output, or put in the conversion filters?
	Audio_Format af;
	af.Channels = channels;
	af.SampleRate = srate;
	af.BitsPerSample = bits;
	af.LittleEndian = true;
	af.SignedSample = true;

	return new RawDecoder(af, datasource);
}

uint32 RawDecoder::doDecode(uint8* buf, uint32 max, uint32 req)
{
	uint32 res = 0;
	do {
		uint32 read = source->read(buf+res, max-res);
		if (read == 0) return res;
		res += read;
	} while (res < req);
	return res;
}
