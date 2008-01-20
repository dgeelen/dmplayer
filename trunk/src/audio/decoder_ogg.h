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
		OGGDecoder(IDataSource* ds);
		~OGGDecoder();
		IDecoder* tryDecode(IDataSource* ds);
		uint32 doDecode(char* buf, uint32 max, uint32 req) { return 0; }; //FIXME: implement in .cpp file

		/* Functions for DataSources */
		ogg_packet* get_packet_from_stream(long stream_id);
		void reset();
	private:
		ogg_sync_state* sync;
// 		ogg_page* page;
// 		ogg_stream_state* stream;
// 		char* buffer;
		std::map<long, struct stream_decoding_state > streams;
		bool all_bos_pages_handled;
		ogg_page* read_page();
		void read_next_page_for_stream(long stream_id);
		void read_next_packet_from_stream(long stream_id);

		IDataSource* datasource;

		void uninitialize();
		void initialize();

		int feed_data();
		int next_page();
		int try_next_page();
};

#endif//DECODER_OGG_H
