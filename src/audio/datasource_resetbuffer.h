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
		void noMoreResets();
		bool exhausted();
		long getpos();
		uint32 getData(uint8* buffer, uint32 len);
	private:
		std::vector<uint8> data; //NOTE: Should be at least MAX_(OGG/VORBIS)_PACKET_SIZE
		IDataSourceRef datasource;
		long bytes_buffered;
		long bytes_read;
		bool done_buffering;
		bool done_resetting;
		std::vector<std::vector<uint8> > history;
		uint32 index_a;
		uint32 index_b;
};


#endif // DATASOURCE
