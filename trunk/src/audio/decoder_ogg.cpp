#include <cstdlib>
#include "decoder_ogg.h"
#include "datasource_oggstream.h"
#include "datasource_resetbuffer.h"
#include "../error-handling.h"
#include <string.h>
#include <vector>
#include <stdio.h>

using namespace std;
//FIXME: Does this mean min_file_size == 8k?
#define BLOCK_SIZE 1024*8

OGGDecoder::OGGDecoder(IDataSourceRef ds) : IDecoder(AudioFormat()) {
	datasource = ds;
	datasource->reset();
	ogg_sync_init(&sync);
	onDestroy.add(boost::bind(&ogg_sync_clear, &sync));
	onDestroy.add(boost::bind(&OGGDecoder::clear_streams, this));
	initialize();
}

void OGGDecoder::setDecoder(IDecoderRef decoder) {
	current_decoder = decoder; //FIXME: Smart Pointer will clean up?
	if(current_decoder) {
		audioformat = current_decoder->getAudioFormat();
	}
}

bool OGGDecoder::exhausted() {
	bool b = true;
	for(map<long, struct stream_decoding_state >::iterator i = streams.begin(); i!=streams.end(); ++i) {
		stream_decoding_state& s = i->second;
		b = b && s.exhausted;
	}
	return b;
}

OGGDecoder::~OGGDecoder() {
	uninitialize();
}

void OGGDecoder::clear_streams() {
	for(map<long, struct stream_decoding_state >::iterator i = streams.begin(); i!=streams.end(); ++i) {
		stream_decoding_state& s = i->second;
		for(list<ogg_packet*>::iterator j=s.packets.begin(); j!=s.packets.end(); ++j) {
			delete *j;
		}
		ogg_stream_destroy(s.stream_state);
	}
	streams.clear();
}

void OGGDecoder::uninitialize() {
	clear_streams();
}

void OGGDecoder::initialize() {
	ogg_sync_reset(&sync);
	read_bos_pages(true);
	eos_count = 0;
	first_stream = true;
	if(streams.size()==0)
		throw Exception("Could not find any BOS pages");
}

void OGGDecoder::reset() {
	if(first_stream) {
		uninitialize();
		if(datasource) datasource->reset();
		initialize();
	}
}

/**
 * Returns the next packet in stream_id
 * @param stream_id Stream idenifier
 * @return The next ogg_packet in the stream idenified by stream_id, or NULL if none is available
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
			return NULL;
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
	bool done = false;
	ogg_packet* packet = new ogg_packet;
	packet->packet = NULL;
	stream_decoding_state& state = streams[stream_id];
	while(!done) {
		int result = ogg_stream_packetout(state.stream_state, packet);
		switch(result) {
			case 0 : { // Stream needs more data to construct a packet
				if(state.exhausted==true) {
					delete packet;
					done=true;
					break;
				}
				read_next_page_for_stream(stream_id);
				break;
			}
			case -1: { // Non-fatal error while extracting packet
				dcerr("Warning: lost sync in packets, skipping some bytes");
				if((packet->packet==NULL)) { /* Simple test if packet is broken, ifso we ignore it */
					break;
				}
			} /* Fallthrough intentional */
			case 1 : { // Successfully extracted a packet
				streams[stream_id].packets.push_back(packet);
				done=true;
				break;
			}
			default: {
				throw Exception("Invalid return value");
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
			if(ogg_stream_pagein(streams[page_serial].stream_state, page)) throw Exception("Could not submit page to stream");
			done = page_serial == stream_id;
			delete page;
		} else done = true;
	}
}

/**
 * Read the next page from the datasource
 * @return an ogg_page there was another page in the datasource, NULL otherwise
 */
ogg_page* OGGDecoder::read_page() {
	return read_page((uint32)-1);
}

ogg_page* OGGDecoder::read_page(uint32 time_out) {
	ogg_page* page = new ogg_page();
	bool done = false;
	while(!done) {
		int page_state = ogg_sync_pageout(&sync, page);
		switch(page_state) {
			case 0: { // needs more data to construct a page
				char* buffer = ogg_sync_buffer(&sync, BLOCK_SIZE);
				long bytes_read = datasource->getData( (uint8*)buffer, BLOCK_SIZE );
				if(ogg_sync_wrote(&sync, bytes_read)) throw Exception("Internal error in libogg!");
				if((bytes_read==0) || ((datasource->getpos() >= (int)time_out ))) {
					if(datasource->exhausted()) {
						delete page;
						// Since we're not going to get any more pages (DataSource is exhausted)
						// we set all streams to exhausted.
						for(map<long, struct stream_decoding_state >::iterator i = streams.begin(); i!=streams.end(); ++i) {
							stream_decoding_state& s = i->second;
							s.exhausted = true;
						}
						return NULL;
					}
				}
				break;
			}
			case -1: { // Non-fatal error while extracting page
				// Note that unlike in the case of extracting a packet
				// it appears we do not yet have a page in this case
				dcerr("Warning: lost sync in pages, skipping some bytes");
				break; //FIXME: Same test as in lost packet?
			}
			case 1 : { // Successfully extracted a page
				if(ogg_page_eos(page)) {
					eos_count++;
					dcerr("Eos marker for stream "<<ogg_page_serialno(page)<<" found for a total of "<<eos_count<<" eos markers");
					streams[ogg_page_serialno(page)].exhausted = true;
				}
				done=true;
				break;
			}
			default: {
				throw Exception("Invalid return value");
				break;
			}
		}
	}
	return page;
}

IDecoderRef OGGDecoder::find_decoder() {
	IDecoderRef decoder;
	for(map<long, stream_decoding_state>::iterator i = streams.begin(); i!=streams.end(); ++i) {
		dcerr("found a stream with ID " << i->second.stream_state->serialno);
		boost::shared_ptr<OGGDecoder> ptr = shared_from_this();
		OGGStreamDataSourceRef oggs = OGGStreamDataSourceRef(new OGGStreamDataSource(OGGDecoderRef(ptr), i->second.stream_state->serialno));
		ResetBufferDataSourceRef ds = ResetBufferDataSourceRef(new ResetBufferDataSource(oggs));
		decoder = IDecoder::findDecoder(ds);
		if (decoder) {
			ds->noMoreResets();
			break;
		}
	}
	return decoder;
}

/* After this all bos pages should be in streams */
void OGGDecoder::read_bos_pages(bool from_start_of_stream) {
	all_bos_pages_handled = false;
	ogg_page* page;
	bool done = false;
	while((!done) && ((datasource->getpos() < 1024*16) || !from_start_of_stream )) {
		page = read_page(1024*16);
		all_bos_pages_handled = true;
		if(page) {
			long page_serial = ogg_page_serialno(page);
			if(ogg_page_bos(page)) { // This page belongs to a new stream
				assert(streams.find(page_serial) == streams.end());
				streams[page_serial] = stream_decoding_state();
				streams[page_serial].exhausted = false;
				streams[page_serial].stream_state = new ogg_stream_state();
				if(ogg_stream_init(streams[page_serial].stream_state, page_serial))
					throw Exception("libogg: could not initialize stream");
				all_bos_pages_handled = false;
			}
			if(ogg_stream_pagein(streams[page_serial].stream_state, page)) throw Exception("Could not submit page to stream");
		}
		done = all_bos_pages_handled;
		delete page;
	}
}

uint32 OGGDecoder::getData(uint8* buf, uint32 len) {
	if(!current_decoder) return 0;
	uint32 todo = len;
	while(1) {
		uint32 n = current_decoder->getData(buf + len - todo, todo);
		todo -= n;
		if(todo==0) break;
		if(n==0) break;
	}
	if(!todo) return len;
	else if(streams.size() == eos_count) { // All streams have been closed, now new streams are allowed
		dcerr("Trying new stream...");
		eos_count=0;
		clear_streams();
		read_bos_pages(false);
		if(streams.size()) { // We found new BOS pages
			dcerr("found, setting new decoder");
			first_stream = false;
			setDecoder(find_decoder());
			return getData(buf,len);
		}
		else {
			dcerr("Not found");
			setDecoder(IDecoderRef());
		}
	}
	return 0;
}

IDecoderRef OGGDecoder::tryDecode(IDataSourceRef ds) {
	try {
		OGGDecoderRef oggd(new OGGDecoder(ds));
		IDecoderRef decodable = oggd->find_decoder();
		if(decodable) {
			oggd->setDecoder(decodable);
			return oggd;
		}
		return IDecoderRef();;
	}
	catch (Exception& e) {
		VAR_UNUSED(e); // in debug mode
		dcerr("OGG tryDecode failed: " << e.what());
		return IDecoderRef();
	}
}
