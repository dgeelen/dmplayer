#include "decoder_zlib.h"
#include "../error-handling.h"
#include "../util/ScopeExitSignal.h"
#include <string>

extern "C" {
#include <zlib.h>
}

class ZLibDataSource: public IDataSource
{
	public:
		static const int BUFSIZE = 100*1024;
		IDataSourceRef datasource;
		uint8 ibuf[BUFSIZE];
		uint8 obuf[BUFSIZE];
		z_stream strm;

		size_t istart;
		size_t istop;
		size_t ostop;

		size_t pos;

		ScopeExitSignal onDestroy; // should be last data member

		ZLibDataSource(IDataSourceRef ds)
		: datasource(ds)
		{
			istart = 0;
			istop = 0;
			ostop = 0;
			pos = 0;

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.avail_in = 0;
			strm.avail_out = 0;
			int ret = inflateInit2(&strm, 15 + 32); // 15 window size, 32 allow gzip
			if (ret != Z_OK) throw std::runtime_error("init failed");
			onDestroy.add(boost::bind(&inflateEnd, &strm));

			ostop = getData(obuf, BUFSIZE);
			reset();
		};

		virtual uint32 getpos()
		{
			return pos;
		}

		virtual bool exhausted()
		{
			return pos >= ostop && !datasource;
		}

		virtual void reset()
		{
			if (pos > ostop) throw std::runtime_error("Failed reset");
			pos = 0;
		}

		virtual uint32 getData(uint8* buffer, uint32 len)
		{
			uint32 done = 0;
			if (pos < ostop) {
				uint32 copy = ostop-pos;
				if (len < copy) copy = len;
				memcpy(buffer, obuf+pos, copy);
				done += copy;
				pos += copy;
				if (done == len) return done;
			}

			if (!datasource) return done;

			while (done < len) {
				if (istart == istop) {
					istart = 0;
					istop = datasource->getData(ibuf, BUFSIZE);
				}

				strm.next_in = ibuf+istart;
				strm.avail_in = istop-istart;
				strm.next_out = buffer+done;
				strm.avail_out = len-done;

				int ret = inflate(&strm, 0);
				if (ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END) throw std::runtime_error("inflate failed");

				uint32 rdone = (istop-istart)-strm.avail_in;
				uint32 wdone = (len-done)-strm.avail_out;

				istart += rdone;
				done += wdone;
				pos += wdone;
				if (ret == Z_STREAM_END) {
					datasource.reset();
					return done;
				}
			}

			return done;
		}
};
typedef boost::shared_ptr<ZLibDataSource> ZLibDataSourceRef;

IDecoderRef ZLibDecoder::tryDecode(IDataSourceRef datasource)
{
	/* first do some quick checks to see if we might be able to handle this */
	uint8 tbuf[2];
	if (datasource->getDataRetry(tbuf,2) != 2) return IDecoderRef();
	bool is_gzip = (tbuf[0] == 0x1f) && (tbuf[1] == 0x8b); // gzip magix from http://www.ietf.org/rfc/rfc1952.txt
	bool is_zlib = (((tbuf[0] << 8) | tbuf[1]) % 31 == 0); // zlib magix from http://www.ietf.org/rfc/rfc1950.txt
	if (!is_gzip && !is_zlib) return IDecoderRef();
	datasource->reset();

	/* now actually try to decompress the stream */
	try {
		IDecoderRef ret = IDecoder::findDecoder(ZLibDataSourceRef(new ZLibDataSource(datasource)));
		return ret;
	} catch (std::exception& e) {
		VAR_UNUSED(e); // in debug mode
		dcerr("ZLibDecoder tryDecode failed: " << e.what());
		return IDecoderRef();
	}
}
