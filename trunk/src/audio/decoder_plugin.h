#ifndef DECODER_PLUGIN_H
#define DECODER_PLUGIN_H

#include <boost/function.hpp>

#include "decoder_interface.h"
#include "datasource_interface.h"

#include "plugin_util.h"

#include "../util/DynLoadLibrary.h"

class PluginDecoder: public DecoderFromPPVoid {
	private:
		DynamicLibrary dll;
		IDataSourceRef source;

		static IDecoderRef tryDecodePath(IDataSourceRef datasource, boost::filesystem::path path, int resurse = 0);
	public:
		PluginDecoder(boost::filesystem::path dllname, IDataSourceRef source_);
		~PluginDecoder();
		static IDecoderRef tryDecode(IDataSourceRef datasource);
};

#endif//DECODER_PLUGIN_H
