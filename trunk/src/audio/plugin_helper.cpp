#include "plugin_helper.h"

#include "datasource_interface.h"

#include DECODER_HEADER

#include <boost/shared_ptr.hpp>

namespace {
	class WrappedDataSource: public IDataSource {
		public:
			void* source;
			source_callback_getData_cbt cb_getdata;
			source_callback_reset_cbt cb_reset;
			source_callback_exhausted_cbt cb_exhausted;
			source_callback_getpos_cbt cb_getpos;

		public:
			virtual uint32 getpos() {
				return cb_getpos(source);
			}

			virtual bool exhausted()
			{
				return cb_exhausted(source);
			}

			virtual void reset()
			{
				cb_reset(source);
			}

			virtual uint32 getData(uint8* buffer, uint32 len)
			{
				return cb_getdata(source,buffer,len);
			}
	};

	struct WrappedDecoder {
		IDecoderRef impl;
	};
}

extern "C" const char* getplugintype()
{
	return "Decoder-1";
}

extern "C" DLL_PUBLIC void* create(void* s,source_callback_getData_cbt cb1,source_callback_reset_cbt cb2,source_callback_exhausted_cbt cb3,source_callback_getpos_cbt cb4)
{
	boost::shared_ptr<WrappedDataSource> ds(new WrappedDataSource());
	ds->source = s;
	ds->cb_getdata = cb1;
	ds->cb_reset = cb2;
	ds->cb_exhausted = cb3;
	ds->cb_getpos = cb4;

	ds->reset();
	IDecoderRef dec = DECODER_CLASS::tryDecode(ds);

	if (!dec) return NULL;

	WrappedDecoder* wrapper = new WrappedDecoder();
	wrapper->impl = dec;

	return wrapper;
}

extern "C" void destroy(void* w)
{
	delete (WrappedDecoder*)w;
}

extern "C" uint32 getdata(void* w,uint8* buf,uint32 max)
{
	try {
		return ((WrappedDecoder*)w)->impl->getData(buf, max);
	} catch (...) {
		return 0xffffffff;
	}
}

extern "C" bool exhausted(void* w)
{
	return ((WrappedDecoder*)w)->impl->exhausted();
}

extern "C" DLL_PUBLIC void getaudioformat(void* w, uint32* a,uint32* b,uint32* c,bool* d,bool* e,bool* f)
{
	*a = ((WrappedDecoder*)w)->impl->getAudioFormat().SampleRate;
	*b = ((WrappedDecoder*)w)->impl->getAudioFormat().Channels;
	*c = ((WrappedDecoder*)w)->impl->getAudioFormat().BitsPerSample;
	*d = ((WrappedDecoder*)w)->impl->getAudioFormat().SignedSample;
	*e = ((WrappedDecoder*)w)->impl->getAudioFormat().LittleEndian;
	*f = ((WrappedDecoder*)w)->impl->getAudioFormat().Float;
}
