#ifndef PLUGIN_UTIL_H
#define PLUGIN_UTIL_H

#include "../types.h"
#include "datasource_interface.h"
#include "decoder_interface.h"

typedef void** ppvoid_t;

ppvoid_t IDataSourceToPPVoid(IDataSourceRef);
IDataSourceRef PPVoidToIDataSource(ppvoid_t);

ppvoid_t IDecoderToPPVoid(IDecoderRef);
IDecoderRef PPVoidToIDecoder(ppvoid_t);

class DecoderFromPPVoid: public IDecoder
{
	private:
		ppvoid_t obj;
	protected:
		void destroy();
		void init(ppvoid_t obj_);
	public:
		DecoderFromPPVoid(ppvoid_t obj_);
		~DecoderFromPPVoid();

		uint32 getData(uint8* buf, uint32 max);
		bool exhausted();
};

#endif//PLUGIN_UTIL_H
