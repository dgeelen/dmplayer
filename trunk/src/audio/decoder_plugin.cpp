#include "decoder_plugin.h"
#include "../error-handling.h"
#include "../util/ScopeExitSignal.h"
#include <string>

#include <boost/filesystem.hpp>

#include "decoder_plugin_legacy.h"

namespace {
	ppvoid_t find_decoder_callback(ppvoid_t dsobj)
	{
		IDataSourceRef ds = PPVoidToIDataSource(dsobj);
		IDecoderRef dec = IDecoder::findDecoder(ds);
		if (!dec) return NULL;
		return IDecoderToPPVoid(dec);
	};
};

PluginDecoder::PluginDecoder(boost::filesystem::path dllname, IDataSourceRef source_)
: DecoderFromPPVoid(NULL), dll(dllname), source(source_)
{
	source->reset();

	boost::function<const char*()> df_type;
	dll.getFunction("getplugintype", df_type);

	std::string type(df_type());

	ppvoid_t decoder_obj = NULL;
	if (type == "Decoder-1") {
		decoder_obj = decoder_plugin_legacy_1(dll, source);
	} else if (type == "Decoder-2") {
		typedef ppvoid_t (*find_decoder_callback_type)(ppvoid_t);
		boost::function<ppvoid_t(ppvoid_t)> create_decoder;
		boost::function<void(find_decoder_callback_type)> set_find_decoder_callback;

		if (!dll.getFunction("create_decoder", create_decoder)
		 || !dll.getFunction("set_find_decoder_callback", set_find_decoder_callback)
		 ) throw std::runtime_error("Failed to load functions");

		set_find_decoder_callback(&find_decoder_callback);

		decoder_obj = create_decoder(IDataSourceToPPVoid(source));
	} else
		throw std::runtime_error("Invalid plugin type");

	if (!decoder_obj) throw std::runtime_error("Failed to create decoder_obj");
	init(decoder_obj);
}

PluginDecoder::~PluginDecoder()
{
	// we can't call the destroy callback after ~DynamicLibrary for 'dll' unloads the library
	// so we call destroy() here to make sure it happens before the library is unloaded
	destroy();
}

IDecoderRef PluginDecoder::tryDecodePath(IDataSourceRef datasource, boost::filesystem::path path, int recurse)
{
	dcerr("PluginDecoder: Trying: " << path);
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
