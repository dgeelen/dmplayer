#include "decoder_oggvorbisfile.h"

size_t dsread(void* data, size_t s1, size_t s2, void* ds) {
	IDataSource* rds = (IDataSource*)ds;
	size_t s = s1*s2;
	return rds->read((char*)data, s);
}

static ov_callbacks OV_CALLBACKS_IDATASOURCE = {
  (size_t (*)(void *, size_t, size_t, void *))  dsread,
  (int (*)(void *, ogg_int64_t, int))           NULL,
  (int (*)(void *))                             NULL,
  (long (*)(void *))                            NULL
};

OGGVorbisFileDecoder::OGGVorbisFileDecoder()
{
	this->oggFile = NULL;
}

OGGVorbisFileDecoder::OGGVorbisFileDecoder(OggVorbis_File* oggFile)
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

	return new OGGVorbisFileDecoder(oggFile);
}

uint32 OGGVorbisFileDecoder::doDecode(char* buf, uint32 max, uint32 req)
{
	return ov_read(oggFile, buf, max, 0, 2, 1, &bitStream);
}
