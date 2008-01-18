#include <cstdlib>
#include "decoder_libogg.h"
#include "../error-handling.h"

OGGDecoder::OGGDecoder() : IDecoder() {
	ogg_sync_state* sync = NULL;
}

OGGDecoder::~OGGDecoder() {

}

//FIXME: Does this mean min_file_size == 8k?
#define BLOCK_SIZE 1024

IDecoder* OGGDecoder::tryDecode(IDataSource* ds) {
	/* Initialize libogg */
	dcerr("Initializing libogg");
	sync = new ogg_sync_state();
	page = new ogg_page();
	ogg_sync_init(sync);

	dcerr("Attempting to read a page");

	int page_state = 0;
	while(!page_state) {
		dcerr("1");
		page_state = ogg_sync_pageout(sync, page);
		dcerr("2");
		if(page_state==-1) dcerr("libogg: sync lost, skipping some bytes");
		dcerr("3");
		buffer = ogg_sync_buffer(sync, BLOCK_SIZE);
		dcerr("4");
		int bytes_read = ds->read( buffer, BLOCK_SIZE );
		if(!bytes_read) throw "EOF while looking for a page!";
		dcerr("5");
		if(ogg_sync_wrote(sync, bytes_read)) throw "Internal error in libogg!";
		dcerr("6");
	}
	dcerr("Got a OGG page!");

	/* Un-initialize libogg */
	dcerr("Un-initializing libogg");
	delete page;
	ogg_sync_destroy(sync); sync = NULL; //ogg_sync_destroy delete's it for us?!
	/* return to indicate success/failure */
	dcerr("");
	return this;
}
