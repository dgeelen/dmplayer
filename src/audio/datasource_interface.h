#ifndef DATASOURCE_INTERFACE_H
#define DATASOURCE_INTERFACE_H

class IDataSource {
	public:
		IDataSource() {};
		virtual ~IDataSource() {};

		virtual void reset() = 0;

		virtual int read(char* buffer, int len) = 0;
};

#endif//DATASOURCE_INTERFACE_H