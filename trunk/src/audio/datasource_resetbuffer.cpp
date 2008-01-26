#include "datasource_resetbuffer.h"
#include "../error-handling.h"
#include <algorithm>
using namespace std;

#define MAX_BUFFER_SIZE (128*1024)
#define MAX_CHUNK_SIZE (1<<13)
ResetBufferDataSource::ResetBufferDataSource( IDataSourceRef ds ) {
	datasource = ds;
	bytes_buffered = 0;
	bytes_read = 0;
	index_a = 0;
	index_b = 0;
	done_buffering = false;
	done_resetting = false;
	data.resize(MAX_CHUNK_SIZE);
}

ResetBufferDataSource::~ResetBufferDataSource( ) {
}

void ResetBufferDataSource::reset() {
	if (!done_resetting) {
		bytes_read =0;
		index_a = 0;
		index_b = 0;
	} else
		throw IOException("Exception: resetting with a corrupt buffer!");
}

void ResetBufferDataSource::noMoreResets()
{
	done_resetting = true;
}

bool ResetBufferDataSource::exhausted() {
	if (done_buffering || index_a == history.size())
		return datasource->exhausted();
	return false;
}

long ResetBufferDataSource::getpos() {
	if (done_buffering)
		return datasource->getpos();
	return bytes_read;
}

uint32 ResetBufferDataSource::getData(uint8* buf, uint32 len) {
	if (done_buffering)
		return datasource->getData(buf, len);

	if (index_a == history.size()) {
		if (bytes_buffered>MAX_BUFFER_SIZE)
			done_resetting = true;
		if (done_resetting) {
			dcerr("Done resetting, passthrough now");
			done_buffering = true;
			history.clear(); history.reserve(0);
			data.clear();    data.reserve(0);
			return getData(buf, len);
		} else {
			uint32 read = datasource->getData(&data[0], 1<<13);
			if(read) {
				history.push_back(vector<uint8>());
				history[index_a].resize(read);
				memcpy(&(history[index_a][0]), &data[0], read);
				bytes_buffered += read;
			}
		}
	}

	/* Data must be available in buffer */
	if (index_a == history.size())
		return 0;

	uint32 n = min(len, history[index_a].size() - index_b);
	memcpy(buf, &(history[index_a][index_b]), n);
	index_b += n;
	bytes_read += n;
	if (index_b == history[index_a].size()) {
		++index_a;
		index_b=0;
	}
	return n;
}
