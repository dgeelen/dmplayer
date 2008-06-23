#include "decoder_wave.h"
#include <string>

WaveDecoder::WaveDecoder(AudioFormat af, IDataSourceRef ds)
	: IDecoder(af)
	, source(ds)
{
}

WaveDecoder::~WaveDecoder()
{
}

bool WaveDecoder::exhausted() {
	return source->exhausted();
}

IDecoderRef WaveDecoder::tryDecode(IDataSourceRef datasource)
{
	datasource->reset();
	uint8 hdr[48];
	if (datasource->getData(hdr, 48) != 48) return IDecoderRef();
#define STRCHECK(x, str) for (unsigned int i = 0; i < strlen(str); ++i) if (hdr[x+i] != str[i]) return IDecoderRef();

	STRCHECK( 0, "RIFF");
	STRCHECK( 8, "WAVE");
	STRCHECK(12, "fmt ");
	STRCHECK(36, "data");

	uint16 fmt      = hdr[20] + (hdr[21] << 8);
	uint16 channels = hdr[22] + (hdr[23] << 8);
	uint32 srate    = hdr[24] + (hdr[25] << 8) + (hdr[26] << 16) + (hdr[27] << 24);
	uint16 bits     = hdr[34] + (hdr[35] << 8);

	if (fmt != 1)       return IDecoderRef();

	AudioFormat af;
	af.Channels = channels;
	af.SampleRate = srate;
	af.BitsPerSample = bits;
	af.LittleEndian = true;
	af.SignedSample = true;

	return IDecoderRef(new WaveDecoder(af, datasource));
}

uint32 WaveDecoder::getData(uint8* buf, uint32 len)
{
	uint32 res = 0;
	do {
		uint32 read = source->getData(buf+res, len-res);
		if (read == 0) return res;
		res += read;
	} while (res < len);
	return res;
}
