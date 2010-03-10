#include "plugin_util.h"

#include <stdexcept>

namespace {
	void datasource_destroy(ppvoid_t obj)
	{
		delete (IDataSourceRef*)obj[0];
		delete obj;
	}

	uint32 datasource_getdata(ppvoid_t obj, uint8* buf, uint32 max)
	{
		try {
			if (max == 0xffffffff) --max;
			return (*((IDataSourceRef*)obj[0]))->getData(buf, max);
		} catch (...) {
			return 0xffffffff;
		}
	}

	void datasource_reset(ppvoid_t obj)
	{
		(*((IDataSourceRef*)obj[0]))->reset();
	}

	bool datasource_exhausted(ppvoid_t obj)
	{
		return (*((IDataSourceRef*)obj[0]))->exhausted();
	}

	uint32 datasource_getpos(ppvoid_t obj)
	{
		return (*((IDataSourceRef*)obj[0]))->getpos();
	}
};

ppvoid_t IDataSourceToPPVoid(IDataSourceRef ds)
{
	ppvoid_t obj = new void*[6];
	obj[0] = (void*)new IDataSourceRef(ds);
	obj[1] = (void*)&datasource_destroy;
	obj[2] = (void*)&datasource_getdata;
	obj[3] = (void*)&datasource_reset;
	obj[4] = (void*)&datasource_exhausted;
	obj[5] = (void*)&datasource_getpos;
	return obj;
}

class DataSourceFromPPVoid: public IDataSource
{
	private:
		ppvoid_t obj;
	public:
		DataSourceFromPPVoid(ppvoid_t obj_);

		~DataSourceFromPPVoid();

		uint32 getData(uint8* buffer, uint32 len);
		void reset();
		bool exhausted();
		uint32 getpos();
};

DataSourceFromPPVoid::DataSourceFromPPVoid(ppvoid_t obj_)
	: obj(obj_)
{
}

DataSourceFromPPVoid::~DataSourceFromPPVoid()
{
	if (!obj) return;
	typedef void (*cbtype)(ppvoid_t);
	((cbtype)obj[1])(obj);
	obj = NULL;
}

uint32 DataSourceFromPPVoid::getData(uint8* buffer, uint32 len)
{
	typedef uint32 (*cbtype)(ppvoid_t, uint8*, uint32);
	if (len == 0xffffffff) --len;
	uint32 ret = ((cbtype)obj[2])(obj, buffer, len);
	if (ret == 0xffffffff) throw std::runtime_error("DataSourceFromPPVoid::getData");
	return ret;
}

void DataSourceFromPPVoid::reset()
{
	typedef void (*cbtype)(ppvoid_t);
	((cbtype)obj[3])(obj);
}

bool DataSourceFromPPVoid::exhausted()
{
	typedef bool (*cbtype)(ppvoid_t);
	return ((cbtype)obj[4])(obj);
}

uint32 DataSourceFromPPVoid::getpos()
{
	typedef uint32 (*cbtype)(ppvoid_t);
	return ((cbtype)obj[5])(obj);
}

IDataSourceRef PPVoidToIDataSource(ppvoid_t obj)
{
	if (!obj) return IDataSourceRef();
	return IDataSourceRef(new DataSourceFromPPVoid(obj));
}

namespace {
	void decoder_destroy(ppvoid_t obj)
	{
		delete (IDecoderRef*)obj[0];
		delete obj;
	}

	uint32 decoder_getdata(ppvoid_t obj, uint8* buf, uint32 max)
	{
		try {
			if (max == 0xffffffff) --max;
			return (*((IDecoderRef*)obj[0]))->getData(buf, max);
		} catch (...) {
			return 0xffffffff;
		}
	}

	bool decoder_exhausted(ppvoid_t obj)
	{
		return (*((IDecoderRef*)obj[0]))->exhausted();
	}

	void decoder_getaudioformat(ppvoid_t obj, uint32* a, uint32* b, uint32* c, bool* d, bool* e, bool* f)
	{
		AudioFormat af = (*((IDecoderRef*)obj[0]))->getAudioFormat();
		*a = af.SampleRate;
		*b = af.Channels;
		*c = af.BitsPerSample;
		*d = af.SignedSample;
		*e = af.LittleEndian;
		*f = af.Float;
	}
};

ppvoid_t IDecoderToPPVoid(IDecoderRef ds)
{
	ppvoid_t obj = new void*[5];
	obj[0] = (void*)new IDecoderRef(ds);
	obj[1] = (void*)&decoder_destroy;
	obj[2] = (void*)&decoder_getdata;
	obj[3] = (void*)&decoder_exhausted;
	obj[4] = (void*)&decoder_getaudioformat;
	return obj;
}

DecoderFromPPVoid::DecoderFromPPVoid(ppvoid_t obj_)
: obj(NULL)
{
	init(obj_);
}

DecoderFromPPVoid::~DecoderFromPPVoid()
{
	destroy();
}

void DecoderFromPPVoid::destroy()
{
	if (!obj) return;
	typedef void (*cbtype)(ppvoid_t);
	((cbtype)obj[1])(obj);
	obj = NULL;
}

void DecoderFromPPVoid::init(ppvoid_t obj_)
{
	destroy();
	obj = obj_;
	if (!obj) return;
	// getaudioformat
	typedef void (*cbtype)(ppvoid_t, uint32*, uint32*, uint32*, bool*, bool*, bool*);
	((cbtype)obj[4])(obj,
		&audioformat.SampleRate,
		&audioformat.Channels,
		&audioformat.BitsPerSample,
		&audioformat.SignedSample,
		&audioformat.LittleEndian,
		&audioformat.Float
	);
}

uint32 DecoderFromPPVoid::getData(uint8* buf, uint32 max)
{
	typedef uint32 (*cbtype)(ppvoid_t, uint8*, uint32);
	if (max == 0xffffffff) --max;
	uint32 ret = ((cbtype)obj[2])(obj, buf, max);
	if (ret == 0xffffffff) throw std::runtime_error("DecoderFromPPVoid::getData");
	return ret;

}

bool DecoderFromPPVoid::exhausted()
{
	typedef bool (*cbtype)(ppvoid_t);
	return ((cbtype)obj[3])(obj);
}

IDecoderRef PPVoidToIDecoder(ppvoid_t obj)
{
	if (!obj) return IDecoderRef();
	return IDecoderRef(new DecoderFromPPVoid(obj));
}
