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
		bool exhausted();
		int read(char* const buffer, int len) = 0;
		ogg_packet* read();
	private:
		OGGDecoder* decoder;
		long stream_id;
};


#endif // DATASOURCE_OGGSTREAM_H
