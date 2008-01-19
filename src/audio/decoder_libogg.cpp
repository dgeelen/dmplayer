#include <cstdlib>
#include "decoder_libogg.h"
#include "../error-handling.h"

OGGDecoder::OGGDecoder() : IDecoder() {
	ogg_sync_state* sync = NULL;
}

OGGDecoder::~OGGDecoder() {

}

//FIXME: Does this mean min_file_size == 8k?
#define BLOCK_SIZE 1024*8

IDecoder* OGGDecoder::tryDecode(IDataSource* ds) {
	IDecoder* decoder = NULL;
	/* Initialize libogg */
	dcerr("Initializing libogg");
	sync = new ogg_sync_state();
	page = new ogg_page();
	ogg_sync_init(sync);

	dcerr("Attempting to read a page");
	bool done = false;
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
			stream = new ogg_stream_state;
			ogg_packet* p = new ogg_packet;
			if(ogg_stream_init(stream, ogg_page_serialno(page))) throw "libogg: could not initialize stream";
			if(ogg_stream_pagein(stream, page)) dcerr("Could not submit page to stream");
			while(ogg_stream_packetout(stream, p)!=0) {
				dcerr("New packet");
			}
			delete p;
			delete stream;
			}
		else {
			dcerr("Page is not BOS!");
		}
	}
// 	done = true; //decode just 1 page
	}
	/* Un-initialize libogg */
	dcerr("Un-initializing libogg");
	delete page;
	ogg_sync_destroy(sync); sync = NULL; //ogg_sync_destroy delete's it for us?!
	/* return to indicate success/failure */
	dcerr("");
	return decoder;
}
