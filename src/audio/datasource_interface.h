#ifndef DATASOURCE_INTERFACE_H
#define DATASOURCE_INTERFACE_H

#include <cstdio> // for NULL

class IDataSource {
	public:
		IDataSource() {};
		virtual ~IDataSource() {};

		virtual void reset() = 0;

		virtual int read(char* const & buffer, int len) = 0;
};

#endif//DATASOURCE_INTERFACE_H
