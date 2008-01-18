#include "datasource_filereader.h"
#include <iostream>

/** @brief FileReaderDataSource
  *
  * @todo: document this function
  */
 FileReaderDataSource::FileReaderDataSource(std::string FileName)
{
	FileHandle = fopen(FileName.c_str(), "rb");
	std::cerr << FileHandle;
	if (FileHandle == NULL)
		throw "Could not open " + FileName;
}

/** @brief reset
  *
  * @todo: document this function
  */
void FileReaderDataSource::reset()
{
	fseek (FileHandle , 0 , SEEK_SET);
}

/** @brief read
  *
  * @todo: document this function
  */
int FileReaderDataSource::read(char* buffer, int len)
{
	return fread (buffer, 1, len, FileHandle);
}

