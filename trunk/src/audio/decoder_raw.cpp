#include "decoder_raw.h"

RawDecoder::RawDecoder()
{
	source = NULL;
}

RawDecoder::RawDecoder(IDataSource* source)
{
	this->source = source;
}

RawDecoder::~RawDecoder()
{
}

IDecoder* RawDecoder::tryDecode(IDataSource* datasource)
{
	return false; // everything is raw. but usually still not what we want
}

uint32 RawDecoder::doDecode(char* buf, uint32 max, uint32 req)
{
	return 0;
}