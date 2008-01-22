#ifndef DECODER_OGG_H
#define DECODER_OGG_H

#include "decoder_interface.h"
#include "datasource_interface.h"
#include <ogg/ogg.h>
#include <map>
#include <list>

struct stream_decoding_state {
	bool exhausted;                   // set after last packet is read
	ogg_stream_state* stream_state;
	std::list<ogg_packet*> packets;
};

class OGGDecoder : public IDecoder {
	public:
		OGGDecoder();
		~OGGDecoder();
		IDecoder* tryDecode(IDataSource* ds);
		uint32 getData(uint8* buf, uint32 max);

		/* Functions for DataSources */
		ogg_packet* get_packet_from_stream(long stream_id);
		void reset();
	private:
		void read_bos_pages();
		OGGDecoder(IDataSource* ds);
		IDecoder* find_decoder();

		ogg_sync_state* sync;
		std::map<long, struct stream_decoding_state > streams;
		bool all_bos_pages_handled;
		ogg_page* read_page();
		ogg_page* read_page(uint32 time_out);
		void read_next_page_for_stream(long stream_id);
		void read_next_packet_from_stream(long stream_id);

		IDataSource* datasource;
		long current_stream;
		IDecoder* current_decoder;

		void uninitialize();
		void initialize();
};

#endif//DECODER_OGG_H
