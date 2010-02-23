#ifndef DATASOURCE_INTERFACE_H
#define DATASOURCE_INTERFACE_H

#include "../types.h"
#include <cstdio> // for NULL

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class IDataSource> IDataSourceRef;
class IDataSource {
	public:
		IDataSource() {};
		virtual ~IDataSource() {};

		virtual uint32 getpos() = 0;
		virtual bool exhausted() = 0;
		virtual void reset() = 0;

		virtual uint32 getData(uint8* buffer, uint32 len) = 0;

		uint32 getDataRetry(uint8* buffer, uint32 len)
		{
			uint32 done = 0;
			while (done < len && !exhausted()) {
				uint32 read = getData(buffer+done, len-done);
				done += read;
			}
			return done;
		}
};

#endif//DATASOURCE_INTERFACE_H
