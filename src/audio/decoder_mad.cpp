#include "decoder_mad.h"
#include "../error-handling.h"
#include <algorithm>
#include <iostream>
#include <limits>

MadDecoder::~MadDecoder()
{
	dcerr("shutting down");
	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);
	dcerr("shut down");
}

MadDecoder::MadDecoder(AudioFormat af, IDataSourceRef source)
	: IDecoder(af)
	, datasource(source)
{
	dcerr("New MadDecoder");
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	BytesInOutput = 0;
	BytesInInput = 0;
	start_of_mp3_data = 0;
	eos = false;
	datasource.reset();
}

bool MadDecoder::exhausted() {
	return eos;
}

MadDecoder::MadDecoder(AudioFormat af, IDataSourceRef ds, size_t start_of_mp3_data_)
	: IDecoder(af)
	, datasource(ds)
{
	dcerr("New MadDecoder");
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	BytesInOutput = 0;
	BytesInInput = 0;
	start_of_mp3_data = start_of_mp3_data_;
	eos = false;
	datasource = ds;
	datasource->reset();

	// Fix slow seeking to start of mp3 data due to ID3 tags [HAX]
	while (BytesInInput < start_of_mp3_data && !datasource->exhausted())
		BytesInInput += datasource->getData(input_buffer+BytesInInput, start_of_mp3_data-BytesInInput);
	BytesInInput = 0;
}

void MadDecoder::fill_buffer() {
	while (BytesInInput < INPUT_BUFFER_SIZE && !datasource->exhausted())
		BytesInInput += datasource->getData(input_buffer+BytesInInput, INPUT_BUFFER_SIZE-BytesInInput);
}

IDecoderRef MadDecoder::tryDecode(IDataSourceRef ds)
{
	ds->reset();
	IDecoderRef result;
	uint32 BytesInInput = 0;
	uint8 input_buffer[INPUT_BUFFER_SIZE];
	mad_stream Stream;
	mad_synth Synth;
	mad_frame Frame;
	mad_timer_t Timer;

	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);

	while (BytesInInput < INPUT_BUFFER_SIZE && !ds->exhausted())
		BytesInInput += ds->getData(input_buffer+BytesInInput, INPUT_BUFFER_SIZE-BytesInInput);

	Stream.next_frame = input_buffer; // make sure while loop is entered at least once

	// TODO: Improve this very simple ID3 tag skip
	// see http://www.gigamonkeys.com/book/practical-an-id3-parser.html
	// and http://www.id3.org/id3v2.4.0-structure
// 	for(int i=0; i<9; ++i)
// 		dcerr("input_buffer[" << i << "]" << " = " << (int)(input_buffer[i]));
	if((input_buffer[0] == 'I') && (input_buffer[1] == 'D') && (input_buffer[2] == '3')) {
// 		dcerr("id3");
		// input_buffer[3,4]     == version
		if((input_buffer[3]!=0xff) && (input_buffer[4]!=0xff)) {
// 			dcerr("version");
		// input_buffer[5]       == flags
			if((input_buffer[5]&0x0f)==0) {
// 				dcerr("flags");
			// input_buffer[6,7,8,9] == size
				if(((input_buffer[6]&0x80)|(input_buffer[7]&0x80)|(input_buffer[8]&0x80)|(input_buffer[9]&0x80))==0) {
// 					dcerr("size");
					uint32 length = (input_buffer[6] << (24-3)) | (input_buffer[7] << (16-2)) | (input_buffer[8] << (8-1)) | (input_buffer[9] << (0-0));
					while(length>0 && !ds->exhausted()) {
						if(length>BytesInInput) {
							int read = ds->getData(input_buffer, INPUT_BUFFER_SIZE);
							length -= BytesInInput;
							BytesInInput = read;
						}
						else {
							memmove(input_buffer, input_buffer + length, BytesInInput - length);
							BytesInInput-=length;
							length = 0;
							while (BytesInInput < INPUT_BUFFER_SIZE && !ds->exhausted())
								BytesInInput += ds->getData(input_buffer+BytesInInput, INPUT_BUFFER_SIZE-BytesInInput);
						}
					}
				}
			}
		}
  }

	while (!result && Stream.next_frame < input_buffer+BytesInInput) {
		uint32 BytesLeft = input_buffer+BytesInInput-Stream.next_frame;
		mad_stream_buffer(&Stream, input_buffer+BytesInInput-BytesLeft, BytesLeft);
		Stream.error = MAD_ERROR_NONE;
		mad_frame_decode(&Frame, &Stream);
		if (Stream.error == MAD_ERROR_NONE) {
			BytesLeft = input_buffer+BytesInInput-Stream.next_frame;
			mad_stream_buffer(&Stream, input_buffer+BytesInInput-BytesLeft, BytesLeft);
			mad_frame_decode(&Frame, &Stream);
			if (Stream.error == MAD_ERROR_NONE){
				AudioFormat af;
				af.SampleRate = Frame.header.samplerate;
				af.Channels = MAD_NCHANNELS(&Frame.header);
				af.BitsPerSample = 16;
				af.LittleEndian = true;
				af.SignedSample = true;
				result = IDecoderRef(new MadDecoder(af, ds, BytesInInput-BytesLeft));
			}
		}
		if (Stream.error == MAD_ERROR_BUFLEN)
			break;
	}

	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);

	return result;
}

uint32 MadDecoder::getData(uint8* buf, uint32 len)
{
	uint32 BytesOut = 0;

	// Empty the outputbuffer here if there are still samples left
	if (BytesInOutput > 0) {
		int numcopy = std::min(len, BytesInOutput);
		memcpy(buf, output_buffer, numcopy);
		BytesOut += numcopy;
		BytesInOutput -= numcopy;
		memmove(output_buffer, output_buffer + numcopy, BytesInOutput);
	}

	while (BytesOut < len) { // There are not enough bytes yet decoded

		{	//Fill the inputbuffer as full as possible
			uint toRead = INPUT_BUFFER_SIZE - BytesInInput;
			uint Read = datasource->getData(input_buffer + BytesInInput, toRead);
			BytesInInput += Read;
		}

		if (BytesInInput == 0) {
			eos = datasource->exhausted() && (BytesInOutput == 0);
			return BytesOut;
		}
		//Buffer is sent to MAD to decode to a stream
		mad_stream_buffer(&Stream, input_buffer, BytesInInput);
		Stream.error = MAD_ERROR_NONE;

		//Decode the frame here
		if(mad_frame_decode(&Frame, &Stream)) {
			if (!MAD_RECOVERABLE(Stream.error)) {
				/* FIXME:
				 * This is also thrown when there is simply not enough data available at the moment,
				 * even though it sounds like mad can still continue decoding when data comes in again.
				 */
				dcerr("Unrecoverable error (?)");
				BytesInInput = 0;
				eos = datasource->exhausted(); //should probably be 'true', if the error really is unrecoverable
				return BytesOut;
			}
		}

		if (Stream.next_frame) {
			int BytesLeft = input_buffer + BytesInInput - Stream.next_frame;
			BytesInInput = BytesLeft;
			memmove(input_buffer, Stream.next_frame, BytesLeft);
		} else {
			BytesInInput = 0;
		}
		// Decoding this frame has been succesful

		mad_timer_add(&Timer, Frame.header.duration);
		if (Stream.error == MAD_ERROR_NONE)
			mad_synth_frame(&Synth, &Frame);
 		else
			Synth.pcm.length = 0;

		// [hackmode] Because Mads output is 24 bit PCM, we need to convert everything to 16 bit
		int i;
		for(i = 0; i < Synth.pcm.length && BytesOut < len; ++i)
		{
			for (uint32 j = 0; j < audioformat.Channels; ++j) {
				*((signed short*)(buf+BytesOut)) = MadFixedToSshort(Synth.pcm.samples[j][i]);
				BytesOut += 2;
			}
		}

		for (;i < Synth.pcm.length;++i) {
			for (uint32 j = 0; j < audioformat.Channels; ++j) {
				*((signed short*)(output_buffer+BytesInOutput)) = MadFixedToSshort(Synth.pcm.samples[j][i]);
				BytesInOutput += 2;
			}
		}
	}

	return BytesOut;
}

signed short MadDecoder::MadFixedToSshort(mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The signed short value is formed, after clipping, by the least
	 * significant whole part bit, followed by the 15 most significant
	 * fractional part bits. Warning: this is a quick and dirty way to
	 * compute the 16-bit number, madplay includes much better
	 * algorithms.
	 */

	/* Clipping */
	if(Fixed>=MAD_F_ONE)
		return(std::numeric_limits<short>::max());
	if(Fixed<=-MAD_F_ONE)
		return(std::numeric_limits<short>::min());

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}
