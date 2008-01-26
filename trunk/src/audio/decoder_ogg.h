#ifndef DECODER_OGG_H
#define DECODER_OGG_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <ogg/ogg.h>
#include <map>
#include <list>
#include <boost/enable_shared_from_this.hpp>
#include "../util/ScopeExitSignal.h"

struct stream_decoding_state {
	bool exhausted;                   // set after last packet is read
	ogg_stream_state* stream_state;
	std::list<ogg_packet*> packets;
};

typedef boost::shared_ptr<class OGGDecoder> OGGDecoderRef;
class OGGDecoder : public boost::enable_shared_from_this<OGGDecoder>, public IDecoder {
	public:
		~OGGDecoder();
		static IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 max);

		/* Functions for DataSources */
		ogg_packet* get_packet_from_stream(long stream_id);
		void reset();
	private:
		bool first_stream;
		void read_bos_pages(bool from_start_of_stream);
		long eos_count;
		OGGDecoder(IDataSourceRef ds);
		IDecoderRef find_decoder();
		void setDecoder(IDecoderRef decoder);

		ogg_sync_state sync;
		std::map<long, struct stream_decoding_state > streams;
		bool all_bos_pages_handled;
		ogg_page* read_page();
		ogg_page* read_page(uint32 time_out);
		void read_next_page_for_stream(long stream_id);
		void read_next_packet_from_stream(long stream_id);

		IDataSourceRef datasource;
		long current_stream;
		IDecoderRef current_decoder;

		void clear_streams();
		void uninitialize();
		void initialize();

		ScopeExitSignal onDestroy; // should be last data member
};

#endif//DECODER_OGG_H
