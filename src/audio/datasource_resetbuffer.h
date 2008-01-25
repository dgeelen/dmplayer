#ifndef DATASOURCE_RESETBUFFER_H
#define DATASOURCE_RESETBUFFER_H
#include <vector>
#include "datasource_interface.h"

typedef boost::shared_ptr<class ResetBufferDataSource> ResetBufferDataSourceRef;
class ResetBufferDataSource: public IDataSource {
	public:
		ResetBufferDataSource( IDataSourceRef ds );
		~ResetBufferDataSource();
		void reset();
		bool exhausted();
		long getpos();
		uint32 getData(uint8* buffer, uint32 len);
	private:
		uint8 data[1<<16]; //NOTE: Should be at least MAX_(OGG/VORBIS)_PACKET_SIZE
		IDataSourceRef datasource;
		long bytes_buffered;
		long bytes_read;
		bool buffer_intact;
		std::vector<std::vector<uint8> > history;
		long index_a;
		long index_b;
};


#endif // DATASOURCE
