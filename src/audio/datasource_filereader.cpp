#include "datasource_filereader.h"
#include <iostream>
#include "../error-handling.h"

/** @brief FileReaderDataSource
  *
  * @todo: document this function
  */
 FileReaderDataSource::FileReaderDataSource(std::string FileName)
{
	FileHandle = fopen(FileName.c_str(), "rb");
	if (FileHandle == NULL)
		throw "Could not open " + FileName;
	reset();
}

/** @brief reset
  *
  * @todo: document this function
  */
void FileReaderDataSource::reset()
{
	fseek (FileHandle , 0 , SEEK_SET);
}

bool FileReaderDataSource::exhausted() {
	return feof(FileHandle);
}

/** @brief read
  *
  * @todo: document this function
  */
int FileReaderDataSource::read(char* const buffer, int len)
{
	return fread (buffer, 1, len, FileHandle);
}
