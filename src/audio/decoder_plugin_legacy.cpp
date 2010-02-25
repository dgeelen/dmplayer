#include "decoder_plugin_legacy.h"

// Legacy Stuff for Decoder-1 plugin type
namespace {
	uint32 source_callback_getData(void* source, uint8* buf, uint32 max)
	{
		return ((IDataSource*)source)->getData(buf, max);
	}

	void source_callback_reset(void* source)
	{
		((IDataSource*)source)->reset();
	}

	bool source_callback_exhausted(void* source)
	{
		return ((IDataSource*)source)->exhausted();
	}

	uint32 source_callback_getpos(void* source)
	{
		return ((IDataSource*)source)->getpos();
	}

	void decoder_destroy(ppvoid_t obj)
	{
		typedef void (*cbtype)(void*);
		((cbtype)obj[5])(obj[0]);
		delete obj;
	}

	uint32 decoder_getdata(ppvoid_t obj, uint8* buf, uint32 max)
	{
		try {
			if (max == 0xffffffff) --max;
			typedef uint32 (*cbtype)(void*, uint8*, uint32);
			return ((cbtype)obj[6])(obj[0], buf, max);
		} catch (...) {
			return 0xffffffff;
		}
	}

	bool decoder_exhausted(ppvoid_t obj)
	{
		typedef bool (*cbtype)(void*);
		return ((cbtype)obj[7])(obj[0]);
	}

	void decoder_getaudioformat(ppvoid_t obj, uint32* a, uint32* b, uint32* c, bool* d, bool* e, bool* f)
	{
		typedef bool (*cbtype)(void*, uint32*, uint32*, uint32*, bool*, bool*, bool*);
		((cbtype)obj[8])(obj[0], a, b, c, d, e, f);
	}
}

ppvoid_t decoder_plugin_legacy_1(DynamicLibrary& dll, IDataSourceRef source)
{
	ppvoid_t decoder_obj = NULL;

	typedef uint32 (*source_callback_getdata_cbt)(void*, uint8*, uint32);
	typedef void (*source_callback_reset_cbt)(void*);
	typedef bool (*source_callback_exhausted_cbt)(void*);
	typedef uint32 (*source_callback_getpos_cbt)(void*);

	boost::function<void*(void*,source_callback_getdata_cbt,source_callback_reset_cbt,source_callback_exhausted_cbt,source_callback_getpos_cbt)> dll_create;

	if (!dll.getFunction("create", dll_create)
	 || !dll.getFunction("destroy")
	 || !dll.getFunction("getdata")
	 || !dll.getFunction("exhausted")
	 || !dll.getFunction("getaudioformat")
	 ) throw std::runtime_error("Decoder-1: Failed to load functions");

	void* dlldecoder = dll_create(source.get(), &source_callback_getData, &source_callback_reset, &source_callback_exhausted, &source_callback_getpos);
	if (!dlldecoder) throw std::runtime_error("Decoder-1: Failed to create plugin decoder");

	decoder_obj = new void*[9];
	decoder_obj[0] = (void*)dlldecoder;
	decoder_obj[1] = (void*)&decoder_destroy;
	decoder_obj[2] = (void*)&decoder_getdata;
	decoder_obj[3] = (void*)&decoder_exhausted;
	decoder_obj[4] = (void*)&decoder_getaudioformat;
	decoder_obj[5] = (void*)dll.getFunction("destroy");
	decoder_obj[6] = (void*)dll.getFunction("getdata");
	decoder_obj[7] = (void*)dll.getFunction("exhausted");
	decoder_obj[8] = (void*)dll.getFunction("getaudioformat");

	return decoder_obj;
}
