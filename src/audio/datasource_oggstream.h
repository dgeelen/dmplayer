/* NOTE:
 *  This datasource is intended for internal use by the OGG decoder
 */

#ifndef DATASOURCE_OGGSTREAM_H
#define DATASOURCE_OGGSTREAM_H

#include "datasource_interface.h"
#include "decoder_ogg.h"

typedef boost::shared_ptr<class OGGStreamDataSource> OGGStreamDataSourceRef;
class OGGStreamDataSource: public IDataSource
{
	public:
		OGGStreamDataSource( OGGDecoderRef decoder, long stream_id );
		~OGGStreamDataSource();

		void reset();
		bool is_exhausted;
		bool exhausted();
		long getpos();
		uint32 getData(uint8* buffer, uint32 len);
	private:
		OGGDecoderWeakRef decoderw;
		long stream_id;
		uint32 total_bytes_read;
		uint32 bytes_leftover;
		ogg_packet* packet;
};


#endif // DATASOURCE_OGGSTREAM_H
