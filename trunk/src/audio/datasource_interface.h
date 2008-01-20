#ifndef DATASOURCE_INTERFACE_H
#define DATASOURCE_INTERFACE_H

#include <cstdio> // for NULL

class IDataSource {
	public:
		IDataSource() {};
		virtual ~IDataSource() {};

		virtual long getpos() = 0;
		virtual bool exhausted() = 0;
		virtual void reset() = 0;

		virtual int read(char* const buffer, unsigned long len) = 0;
};

#endif//DATASOURCE_INTERFACE_H
