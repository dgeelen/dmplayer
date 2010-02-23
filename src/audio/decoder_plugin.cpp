#include "decoder_plugin.h"
#include "../error-handling.h"
#include "../util/ScopeExitSignal.h"
#include <string>

#include <boost/filesystem.hpp>

uint32 PluginDecoder::source_callback_getData(void* source, uint8* buf, uint32 max)
{
	return ((IDataSource*)source)->getData(buf, max);
}

void PluginDecoder::source_callback_reset(void* source)
{
	((IDataSource*)source)->reset();
}

bool PluginDecoder::source_callback_exhausted(void* source)
{
	return ((IDataSource*)source)->exhausted();
}

uint32 PluginDecoder::source_callback_getpos(void* source)
{
	return ((IDataSource*)source)->getpos();
}

PluginDecoder::PluginDecoder(boost::filesystem::path dllname, IDataSourceRef source_)
: dll(dllname), dlldecoder(NULL), source(source_)
{
	boost::function<const char*()> df_type;
	dll.getFunction("getplugintype", df_type);

	std::string type(df_type());
	if (type != "Decoder-1") throw std::runtime_error("Invalid plugin type");

	if (!dll.getFunction("create", dll_create)
	 || !dll.getFunction("destroy", dll_destroy)
	 || !dll.getFunction("getdata", dll_getdata)
	 || !dll.getFunction("exhausted", dll_exhausted)
	 || !dll.getFunction("getaudioformat", dll_getaudioformat)
	 ) throw std::runtime_error("Failed to load functions");

	dlldecoder = dll_create(source.get(), &source_callback_getData, &source_callback_reset, &source_callback_exhausted, &source_callback_getpos);
	if (!dlldecoder) throw std::runtime_error("Failed to create plugin decoder");
	dll_getaudioformat(dlldecoder,
		&audioformat.SampleRate,
		&audioformat.Channels,
		&audioformat.BitsPerSample,
		&audioformat.SignedSample,
		&audioformat.LittleEndian,
		&audioformat.Float
	);
}

PluginDecoder::~PluginDecoder()
{
	dll_destroy(dlldecoder);
}

uint32 PluginDecoder::getData(uint8* buf, uint32 max)
{
	if (max == 0xffffffff) --max;
	uint32 ret = dll_getdata(dlldecoder, buf, max);
	if (ret == 0xffffffff) throw std::runtime_error("PluginDecoder Exception");
	return ret;
}

bool PluginDecoder::exhausted()
{
	return dll_exhausted(dlldecoder);
}

IDecoderRef PluginDecoder::tryDecodePath(IDataSourceRef datasource, boost::filesystem::path path, int recurse)
{
	std::cerr << "PluginDecoder: Trying: " << path << "\n";
	IDecoderRef ret;
	if (path.empty()) {
		if (!ret) ret = tryDecodePath(datasource, "plugins/");
		if (!ret) ret = tryDecodePath(datasource, "audio/");
		if (!ret) ret = tryDecodePath(datasource, "src/audio/");
		if (!ret) ret = tryDecodePath(datasource, "audio/Debug/");
		return ret;
	}
	try {
		if (!boost::filesystem::exists(path)) return ret;
		if (boost::filesystem::is_directory(path)) {
			if (recurse < 0) return ret;
			boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
			for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr) {
				ret = tryDecodePath(datasource, itr->path(), recurse-1);
				if (ret) return ret;
			}
		} else {
			ret = IDecoderRef(new PluginDecoder(path, datasource));
		}
		return ret;
	}
	catch (std::exception& e) {
		VAR_UNUSED(e); // in debug mode
		dcerr("PluginDecoder tryDecodePath failed - " << path << " : " << e.what());
		return IDecoderRef();
	}
}

IDecoderRef PluginDecoder::tryDecode(IDataSourceRef datasource)
{
	return tryDecodePath(datasource, "");
}
