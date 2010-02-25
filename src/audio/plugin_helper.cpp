#include "plugin_helper.h"

#include "datasource_interface.h"

#include DECODER_HEADER

#include <boost/shared_ptr.hpp>

namespace {
	find_decoder_callback_type find_decoder_callback = NULL;
};

extern "C" const char* getplugintype()
{
	return "Decoder-2";
}

extern "C" DLL_PUBLIC ppvoid_t create_decoder(ppvoid_t dsobj)
{
	IDataSourceRef ds = PPVoidToIDataSource(dsobj);
	IDecoderRef dec = DECODER_CLASS::tryDecode(ds);
	if (!dec) return NULL;
	return IDecoderToPPVoid(dec);
}

extern "C" DLL_PUBLIC void set_find_decoder_callback(find_decoder_callback_type cb)
{
	if (!find_decoder_callback)
		find_decoder_callback = cb; // assignment of function pointer assumed atomic :O
}


IDecoderRef IDecoder::findDecoder(IDataSourceRef ds)
{
	if (!find_decoder_callback)
		return IDecoderRef();
	return PPVoidToIDecoder(find_decoder_callback(IDataSourceToPPVoid(ds)));
}
