#include "decoder_oggvorbisfile.h"

size_t dsread(void* data, size_t s1, size_t s2, void* ds) {
	IDataSource* rds = (IDataSource*)ds;
	size_t s = s1*s2;
	return rds->read((uint8*)data, s);
}

static ov_callbacks OV_CALLBACKS_IDATASOURCE = {
  (size_t (*)(void *, size_t, size_t, void *))  dsread,
  (int (*)(void *, ogg_int64_t, int))           NULL,
  (int (*)(void *))                             NULL,
  (long (*)(void *))                            NULL
};

OGGVorbisFileDecoder::OGGVorbisFileDecoder() : IDecoder(AudioFormat())
{
	this->oggFile = NULL;
}

OGGVorbisFileDecoder::OGGVorbisFileDecoder(AudioFormat af, OggVorbis_File* oggFile) : IDecoder(af)
{
	this->oggFile = oggFile;
}

OGGVorbisFileDecoder::~OGGVorbisFileDecoder()
{
	if (oggFile) {
		ov_clear(oggFile);
		delete oggFile;
	}
}

IDecoder* OGGVorbisFileDecoder::tryDecode(IDataSource* datasource)
{
	vorbis_info *pInfo;
	datasource->reset();
	OggVorbis_File* oggFile = new OggVorbis_File();
	if (ov_open_callbacks(datasource, oggFile, NULL, 0, OV_CALLBACKS_IDATASOURCE)) {
		ov_clear(oggFile);
		delete oggFile;
		return NULL;
	}
	pInfo = ov_info(oggFile, -1);

	AudioFormat af;
	af.SampleRate = pInfo->rate;
	af.Channels = pInfo->channels;
	af.BitsPerSample = 16;
	af.SignedSample = true;
	af.LittleEndian = true;
	return new OGGVorbisFileDecoder(af, oggFile);
}

uint32 OGGVorbisFileDecoder::doDecode(uint8* buf, uint32 max, uint32 req)
{
	uint32 res = 0;
	do {
		int32 read = ov_read(oggFile, ((char*)buf)+res, max-res, 0, 2, 1, &bitStream);
		if (read <= 0) return res;
		res += read;
	} while (res < req);
	return res;
}
