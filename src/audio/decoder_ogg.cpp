#include <cstdlib>
#include "decoder_ogg.h"
#include "datasource_oggstream.h"
#include "../error-handling.h"
#include <string.h>
#include <vector>
#include <stdio.h>

using namespace std;
//FIXME: Does this mean min_file_size == 8k?
#define BLOCK_SIZE 1024*8

OGGDecoder::OGGDecoder() : IDecoder() {
	initialize();
}

OGGDecoder::OGGDecoder(IDataSource* ds) : IDecoder() {
	initialize();
	datasource = ds;
}

OGGDecoder::~OGGDecoder() {
	uninitialize();
	/* TODO: Discard unclaimed packets */
}

void OGGDecoder::uninitialize() {
// 	if(stream) { /*FIXME: Streams are in some struct */
// 		ogg_stream_destroy(stream);
// 		/* delete stream; Done by libogg?! */
// 		stream = NULL;
// 	}

// 	if(page) {
// 		delete page;
// 		page=NULL;
// 	}

	if(sync) {
		ogg_sync_destroy(sync);
		/* delete sync; Done by libogg?! */
		sync = NULL;
	}
}

/**
 * Returns the next packet in stream_id
 * @param stream_id Stream idenifier
 * @return The next ogg_packet in the stream idenified by stream_id, or an empty packet if none is available
 */
ogg_packet* OGGDecoder::get_packet_from_stream(long stream_id) {
	if(!streams.count(stream_id)) {
		if(all_bos_pages_handled) {
			dcerr("No such stream: "<<stream_id);
			return NULL;
		}
		else {
			read_next_page_for_stream(stream_id);
		}
	}
	if(!streams[stream_id].packets.size()) { // time to read another packet
		read_next_packet_from_stream(stream_id);
		if(!streams[stream_id].packets.size()) { // Stream is exhausted, return empty packet
			dcerr("Stream is empty: "<<stream_id);
			ogg_packet* p = new ogg_packet();
			p->packet= NULL;
			p->bytes =0;
			return p;
		}
	}
	ogg_packet* packet = streams[stream_id].packets.front();
	streams[stream_id].packets.pop_front();
	/* TODO: Discard packets from other streams? */
	return packet;
}

/**
 * Reads a packet from stream_id if possible and places it in the packet list for stream_id
 * @param stream_id
 */
void OGGDecoder::read_next_packet_from_stream(long stream_id) {
	stream_decoding_state& state = streams[stream_id];
	bool done = state.exhausted;
	ogg_packet* packet = new ogg_packet;
	while(!done) {
		int result = ogg_stream_packetout(state.stream_state, packet);
		switch(result) {
			case 0 : { // Stream needs more data to construct a packet
				if(datasource->exhausted()) {
					state.exhausted = true;
					done=true;
					break;
				}
				read_next_page_for_stream(stream_id);
				break;
			}
			case -1: { // Non-fatal error while extracting packet
				dcerr("Warning: lost sync in packets, skipping some bytes");
			} /* Fallthrough intentional */
			case 1 : { // Successfully extracted a packet
				if(packet->e_o_s) state.exhausted = true;
				streams[stream_id].packets.push_back(packet);
				done=true;
				break;
			}
			default: {
				dcerr("Internal error");
				throw "Invalid return value";
				break;
			}
		}
	}
}

/**
 * Reads pages and submits them to the appropriate stream until a page
 * belonging to stream_id is read and submitted
 * @param stream_id
 */
void OGGDecoder::read_next_page_for_stream(long stream_id) {
	bool done = false;
	if(streams.count(stream_id)) done = streams[stream_id].exhausted;
	while(!done) {
		ogg_page* page = read_page();
		if(page) {
			long page_serial = ogg_page_serialno(page);
			if(ogg_page_bos(page)) { // This page belongs to a new stream
				streams[page_serial] = stream_decoding_state();
				streams[page_serial].exhausted = false;
				streams[page_serial].stream_state = new ogg_stream_state();
				if(ogg_stream_init(streams[page_serial].stream_state, page_serial)) throw "libogg: could not initialize stream";
			}
			else {
				all_bos_pages_handled = true;
			}
			if(ogg_stream_pagein(streams[page_serial].stream_state, page)) throw "Could not submit page to stream";
			done = page_serial == stream_id;
		}
	}
}

/**
 * Read the next page from the datasource
 * @return an ogg_page there was another page in the datasource, NULL otherwise
 */
ogg_page* OGGDecoder::read_page() {
	ogg_page* page = new ogg_page();
	bool done = false;
	while(!done) {
		int page_state = ogg_sync_pageout(sync, page);
		switch(page_state) {
			case 0: { // needs more data to construct a page
				char* buffer = ogg_sync_buffer(sync, BLOCK_SIZE);
				int bytes_read = datasource->read( buffer, BLOCK_SIZE );
				if(ogg_sync_wrote(sync, bytes_read)) throw "Internal error in libogg!";
				if(datasource->exhausted()) {
					delete page;
					return NULL;
				}
				break;
			}
			case -1: { // Non-fatal error while extracting page
				// Note that unlike in the case of extracting a packet
				// it appears we do not yet have a page in this case
				dcerr("Warning: lost sync in pages, skipping some bytes");
				break;
			}
			case 1 : { // Successfully extracted a page
				done=true;
				break;
			}
			default: {
				dcerr("Internal error");
				throw "Invalid return value";
				break;
			}
		}
	}
	return page;
}

void OGGDecoder::initialize() {
	sync = new ogg_sync_state();
// 	page = new ogg_page();
// 	stream = new ogg_stream_state();
	ogg_sync_init(sync);
	all_bos_pages_handled = false;
}

void OGGDecoder::reset() {
	uninitialize();
	if(datasource) datasource->reset();
	initialize();
}

//NOTE: We do not (yet, if ever) support concatenated streams
IDecoder* OGGDecoder::tryDecode(IDataSource* ds) {
	reset();
	ds->reset();
	datasource = ds;
	vector<long> stream_ids;
	ogg_page* page;
	bool done = false;
	while(!done && ds->getpos() < 1024*16 ) {
		page = read_page();
		if(page) {
			if(ogg_page_bos(page)) {
				stream_ids.push_back(ogg_page_serialno(page));
			}
			else {
				done = true;
			}
			delete page;
		}
		else {
			done = true;
		}
	}
	IDecoder* result = NULL;

	for(int i=0; i<stream_ids.size(); ++i) {
		dcerr("found a stream with ID " << stream_ids[i]);
		OGGDecoder* oggd = new OGGDecoder(ds);
		OGGStreamDataSource* oggs = new OGGStreamDataSource(oggd, stream_ids[i]);
		for (unsigned int i = 0; i < decoderlist.size(); ++i) {
			IDecoder* dc = decoderlist[i](oggs);
			if (dc) {
				result = dc;
				break;
			}
		}
		if(result) break;
		delete oggs;
		delete oggd;
	}
	datasource = NULL;
	return result;
}
