#ifndef DATASOURCE_INTERFACE_H
#define DATASOURCE_INTERFACE_H

#include "../types.h"
#include <cstdio> // for NULL

class IDataSource {
	public:
		IDataSource() {};
		virtual ~IDataSource() {};

		virtual long getpos() = 0;
		virtual bool exhausted() = 0;
		virtual void reset() = 0;

		virtual uint32 getData(uint8* buffer, uint32 len) = 0;
};

#endif//DATASOURCE_INTERFACE_H
