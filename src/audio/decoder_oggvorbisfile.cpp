#include "decoder_oggvorbisfile.h"

#include "../error-handling.h"

namespace {
	size_t dsread(void* data, size_t s1, size_t s2, void* ds) {
		IDataSource* rds = (IDataSource*)ds;
		size_t s = s1*s2;
		return rds->getData((uint8*)data, s);
	}

	ov_callbacks OV_CALLBACKS_IDATASOURCE = {
		(size_t (*)(void *, size_t, size_t, void *))  dsread,
		(int (*)(void *, ogg_int64_t, int))           NULL,
		(int (*)(void *))                             NULL,
		(long (*)(void *))                            NULL
	};
}

OGGVorbisFileDecoder::OGGVorbisFileDecoder(AudioFormat af, IDataSourceRef ds, OggVorbis_File* f) :
	IDecoder(af),
	datasource(ds),
	oggFile(f)
{
	eos = false;
}

OGGVorbisFileDecoder::~OGGVorbisFileDecoder()
{
	dcerr("Shutting down");
	ov_clear(oggFile);
	delete oggFile;
	dcerr("Shut down");
}

IDecoderRef OGGVorbisFileDecoder::tryDecode(IDataSourceRef datasource)
{
	vorbis_info *pInfo;
	datasource->reset();
	OggVorbis_File* oggFile = new OggVorbis_File();
	if (ov_open_callbacks(datasource.get(), oggFile, NULL, 0, OV_CALLBACKS_IDATASOURCE)) {
		ov_clear(oggFile);
		delete oggFile;
		return IDecoderRef();
	}
	pInfo = ov_info(oggFile, -1);

	AudioFormat af;
	af.SampleRate = pInfo->rate;
	af.Channels = pInfo->channels;
	af.BitsPerSample = 16;
	af.SignedSample = true;
	af.LittleEndian = true;
	return IDecoderRef(new OGGVorbisFileDecoder(af, datasource, oggFile));
}

bool OGGVorbisFileDecoder::exhausted() {
	return datasource->exhausted() && eos;
}

uint32 OGGVorbisFileDecoder::getData(uint8* buf, uint32 len)
{
	uint32 res = 0;
	do {
		int32 read = ov_read(oggFile, ((char*)buf)+res, len-res, 0, 2, 1, &bitStream);
		if (read < 0) dcerr("OGGVorbisFile error from ov_read: " << read);
		if (read <= 0) break;
		res += read;
	} while (res < len);
	eos = res==0;
	return res;
}
