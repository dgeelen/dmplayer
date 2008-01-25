#include "datasource_resetbuffer.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

#define BUFFER_SIZE (128*1024)
ResetBufferDataSource::ResetBufferDataSource( IDataSourceRef ds ) {
	datasource = ds;
	bytes_buffered = 0;
	bytes_read = 0;
	index_a = 0;
	index_b = 0;
	buffer_intact = true;
}

ResetBufferDataSource::~ResetBufferDataSource( ) {
}

void ResetBufferDataSource::reset() {
	if(buffer_intact) {
		bytes_read =0;
		index_a = 0;
		index_b = 0;
	} else dcerr("Warning: resetting with a corrupt buffer!");
}

bool ResetBufferDataSource::exhausted() {
	if(index_a == history.size())
		return datasource->exhausted();
	return false;
}

long ResetBufferDataSource::getpos() {
	return bytes_read;
}

uint32 ResetBufferDataSource::getData(uint8* buf, uint32 len) {
	long result = 0;
	if( index_a < history.size() ) { /* Data is available in buffer */
		uint32 n = min(len, history[index_a].size() - index_b);
		memcpy( buf, &(history[index_a][index_b]), n );
		index_b += n;
		if(index_b==history[index_a].size()) {
			++index_a;
			index_b=0;
		}
		bytes_read += n;
		result = n;
	}
	else {
		uint32 read = datasource->getData(data, 1<<13);
		if(read) {
			history.push_back(vector<uint8>());
			history[index_a].resize(read);
			memcpy(&(history[index_a][0]), data, read);
			bytes_buffered += read;
			result = getData(buf, len);
		}
	}
	while(bytes_buffered>BUFFER_SIZE) {
// 		dcerr("Too many bytes requested, discarding begin of buffer");
		buffer_intact = false;
		--index_a;
		bytes_buffered -= history[0].size();
		history.erase(history.begin());
	}
// 	dcerr("Returning "<<result<<" bytes");
	return result;
}
