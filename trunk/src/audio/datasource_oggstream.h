/* NOTE:
 *  This datasource is intended for internal use by the OGG decoder
 */

#ifndef DATASOURCE_OGGSTREAM_H
#define DATASOURCE_OGGSTREAM_H

#include "datasource_interface.h"
#include "decoder_ogg.h"

class OGGStreamDataSource: public IDataSource
{
	public:
		OGGStreamDataSource( OGGDecoder* decoder, long stream_id );
		~OGGStreamDataSource();

		void reset();
		bool is_exhausted;
		bool exhausted();
		long getpos();
		uint32 getData(uint8* buffer, uint32 len);
		ogg_packet* read();
	private:
		OGGDecoder* decoder;
		long stream_id;
		uint32 total_bytes_read;
};


#endif // DATASOURCE_OGGSTREAM_H
