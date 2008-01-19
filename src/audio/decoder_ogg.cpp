#include <cstdlib>
#include "decoder_ogg.h"
#include "../error-handling.h"
#include <string.h>
#include <vector>
#include <stdio.h>

using namespace std;

OGGDecoder::OGGDecoder() : IDecoder() {
	ogg_sync_state* sync = NULL;
}

OGGDecoder::~OGGDecoder() {

}

//FIXME: Does this mean min_file_size == 8k?
#define BLOCK_SIZE 1024*8

//NOTE: We do not (yet, if ever) support concatenated streams
IDecoder* OGGDecoder::tryDecode(IDataSource* ds) {
	FILE* dump = fopen("ogg_out.dump", "wb");
	ds->reset();
	IDecoder* decoder = NULL;
	/* Initialize libogg */
	dcerr("Initializing libogg");
	sync = new ogg_sync_state();
	page = new ogg_page();
	stream = new ogg_stream_state;
	ogg_sync_init(sync);

	dcerr("Attempting to read a page");
	bool done = false;
	int stream_count = 0;
	unsigned long total_bytes_read = 0;
	while(!done) {
	int page_state=0;
	while(page_state!=1) {
		page_state = ogg_sync_pageout(sync, page);
		if(page_state==-1)
			dcerr("libogg: sync lost, skipping some bytes");
		else if(page_state!=1) {
			buffer = ogg_sync_buffer(sync, BLOCK_SIZE);
			int bytes_read = ds->read( buffer, BLOCK_SIZE );
			if(!bytes_read) {
				dcerr("EOF while looking for a page!");
				done = true;
				break;
			}
			total_bytes_read+=bytes_read;
			if(total_bytes_read >= 16*1024) { // Only inspect the first 16k of a file
				dcerr("Aborting search for OGG pages after "<<total_bytes_read<< " bytes");
				done=true;
				break;
			}
			if(ogg_sync_wrote(sync, bytes_read)) throw "Internal error in libogg!";
		}
	}
	if(page_state) {
		dcerr("Got OGG page nr " << ogg_page_pageno(page));
		dcerr("  begin-of-stream=" << ogg_page_bos(page));
		dcerr("  end-of-stream=" << ogg_page_eos(page));
		dcerr("  version=" << ogg_page_version(page));
		dcerr("  continued=" << ogg_page_continued(page));
		dcerr("  completed=" << ogg_page_packets(page));
		dcerr("  granule-pos="<< ogg_page_granulepos(page));
		dcerr("  serialno="<< ogg_page_serialno(page));
		if(ogg_page_bos(page)) {
			dcerr("Page is a BOS! :D");
			/* We found a begin of stream page, let's see what's inside? */
			++stream_count;
			if(ogg_stream_init(stream, ogg_page_serialno(page))) throw "libogg: could not initialize stream";
			if(ogg_stream_pagein(stream, page)) dcerr("Could not submit page to stream");
			dcerr("Constructing buffer from packets");
			vector<ogg_packet*> packets;
			unsigned long total_length = 0;
			ogg_packet* packet = new ogg_packet;
			while(ogg_stream_packetout(stream, packet)!=0) {
				packets.push_back(packet);
				total_length += packet->bytes;
				packet = new ogg_packet;
			}
			unsigned char header_data[total_length];
			unsigned char *copy_to = header_data;
			for(int i=0; i<packets.size(); ++i) {
				memcpy(copy_to, packets[i]->packet, packets[i]->bytes);
				copy_to+=packets[i]->bytes;
				ogg_packet_clear(packets[i]);
				delete packets[i];
			}
			dcerr("Constructed a buffer of " << total_length << " bytes");
			if(!strncmp((const char*)&header_data[0], "\01vorbis",7)) dcerr("This looks like a vorbis stream!");
		}
		else { //All BOS pages appear in the beginning of the stream
			dcerr("Page is not BOS!, ending search");
			done=true;
		}
	}
	}
	dcerr("Found and processed a total of "<<stream_count<<" streams");
	/* Un-initialize libogg */
	dcerr("Un-initializing libogg");
	delete stream;
	delete page;
	ogg_sync_destroy(sync); sync = NULL; //ogg_sync_destroy delete's it for us?!
	/* return to indicate success/failure */
	dcerr("");
	return decoder;
}
