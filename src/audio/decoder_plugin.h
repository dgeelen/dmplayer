#ifndef DECODER_PLUGIN_H
#define DECODER_PLUGIN_H

#include <boost/function.hpp>

#include "decoder_interface.h"
#include "datasource_interface.h"

#include "../util/DynLoadLibrary.h"

class PluginDecoder: public IDecoder {
	private:
		DynamicLibrary dll;
		void* dlldecoder;
		IDataSourceRef source;

		typedef uint32 (*source_callback_getData_cbt)(void*, uint8*, uint32);
		typedef void (*source_callback_reset_cbt)(void*);
		typedef bool (*source_callback_exhausted_cbt)(void*);
		typedef uint32 (*source_callback_getpos_cbt)(void*);

		static uint32 source_callback_getData(void* source, uint8* buf, uint32 max);
		static void source_callback_reset(void* source);
		static bool source_callback_exhausted(void* source);
		static uint32 source_callback_getpos(void* source);

		boost::function<void*(void*,source_callback_getData_cbt,source_callback_reset_cbt,source_callback_exhausted_cbt,source_callback_getpos_cbt)> dll_create;
		boost::function<void(void*)> dll_destroy;
		boost::function<uint32(void*,uint8*,uint32)> dll_getdata;
		boost::function<bool(void*)> dll_exhausted;
		boost::function<void(void*, uint32*,uint32*,uint32*,bool*,bool*,bool*)> dll_getaudioformat;

		static IDecoderRef tryDecodePath(IDataSourceRef datasource, boost::filesystem::path path, int resurse = 0);
public:
		PluginDecoder(boost::filesystem::path dllname, IDataSourceRef source_);
		~PluginDecoder();
		static IDecoderRef tryDecode(IDataSourceRef datasource);
		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
};

#endif//DECODER_PLUGIN_H
