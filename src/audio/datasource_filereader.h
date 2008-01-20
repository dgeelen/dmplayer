#ifndef DATASOURCE_FILEREADER_H_INCLUDED
#define DATASOURCE_FILEREADER_H_INCLUDED

#include <string>
#include "datasource_interface.h"

class FileReaderDataSource: public IDataSource
{
	public:
		FileReaderDataSource(std::string FileName);
		~FileReaderDataSource() {};

		void reset();
		bool exhausted();
		long getpos();
		int read(char* const buffer, unsigned long len);
	private:
		FILE* FileHandle;
};


#endif // DATASOURCE_FILEREADER_H_INCLUDED
