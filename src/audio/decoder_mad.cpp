#include "decoder_mad.h"
#include "../error-handling.h"
#include <algorithm>
#include <iostream>

MadDecoder::~MadDecoder()
{
	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);
}

MadDecoder::MadDecoder()
{
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	BytesInOutput = 0;
	BytesInInput = 0;
	eos = false;
}

MadDecoder::MadDecoder(IDataSource* ds)
{
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	BytesInOutput = 0;
	BytesInInput = 0;
	eos = false;
	datasource = ds;
	INPUT_BUFFER = new char[INPUT_BUFFER_SIZE];
	OUTPUT_BUFFER = new char[OUTPUT_BUFFER_SIZE];
}

IDecoder* MadDecoder::tryDecode(IDataSource* datasource)
{
	datasource->reset();
	IDecoder* result;
	char* SMALL_INPUT_BUFFER = new char[INPUT_BUFFER_SIZE];
	BytesInInput = datasource->read((uint8*) SMALL_INPUT_BUFFER, INPUT_BUFFER_SIZE);

	mad_stream_buffer(&Stream, (const unsigned char*)SMALL_INPUT_BUFFER, BytesInInput);

	if(!Stream.error){
		result = new MadDecoder(datasource);
	}
	else result = NULL;
	datasource->reset();
	mad_stream_finish(&Stream);

	delete SMALL_INPUT_BUFFER;
	return result;
}

uint32 MadDecoder::doDecode(uint8* buf, uint32 max, uint32 req)
{
	long BytesOut = 0;

	// Empty the outputbuffer here if there are still samples left
	if (BytesInOutput > 0) {
		int numcopy = std::min(max, BytesInOutput);
		memcpy(buf, OUTPUT_BUFFER, numcopy);
		BytesOut += numcopy;
		BytesInOutput -= numcopy;
		memmove(OUTPUT_BUFFER, OUTPUT_BUFFER+numcopy, BytesInOutput);
	}

	while (BytesOut < req) { // There are not enough bytes yet decoded

		{	//Fill the inputbuffer as full as possible
			uint toRead = INPUT_BUFFER_SIZE - BytesInInput;
			uint Read = datasource->read((uint8*) INPUT_BUFFER + BytesInInput, toRead);
			BytesInInput += Read;
			eos = datasource->exhausted();
		}

		if (Stream.buffer == NULL || Stream.next_frame) {

			//Buffer is sent to MAD to decode to a stream
			mad_stream_buffer(&Stream, (const unsigned char*)INPUT_BUFFER, BytesInInput);
			Stream.error = MAD_ERROR_NONE;
		}
		//Decode the frame here

		if(mad_frame_decode(&Frame, &Stream)){
			/// TODO: error handling
		}

		if (Stream.next_frame) {
			int BytesLeft = INPUT_BUFFER + INPUT_BUFFER_SIZE - (char*)Stream.next_frame;
			BytesInInput = BytesLeft;
			memmove(INPUT_BUFFER, Stream.next_frame, BytesLeft);
		} else {
			BytesInInput = 0;
		}
		// Decoding this frame has been succesful

		mad_timer_add(&Timer, Frame.header.duration);
		mad_synth_frame(&Synth, &Frame);

		// [hackmode]
		// Because Mads output is 24 bit PCM, we need to convert everything to 16 bit
		int i;
		for(i = 0; i < Synth.pcm.length && BytesOut+4 <= max; ++i)
		{
			///In this loop we send as much as possible to buf
			signed short* Samplel = (signed short*)(buf+BytesOut);
			signed short* Sampler = (signed short*)(buf+BytesOut+2);
			// Left Channel
			*Samplel = MadFixedToSshort(Synth.pcm.samples[0][i]);

			if (MAD_NCHANNELS(&Frame.header) == 2)
			{
				//Right channel. If the stream is monophonic, then right = left
				*Sampler = 0;//MadFixedToSshort(Synth.pcm.samples[1][i]);
			}
			else *Sampler = *Samplel;
			BytesOut += 4;
		}

		if (BytesOut < req && i < Synth.pcm.length) {

			/** It is possible that the buffer can not be filled with whole samples,
				so we put a part of the next sample in buf.
			*/
			char tbuf[4];
			signed short* Samplel = (signed short*)tbuf;
			signed short* Sampler = (signed short*)(tbuf+2);
			// Left Channel
			*Samplel = MadFixedToSshort(Synth.pcm.samples[0][i]);

			if (MAD_NCHANNELS(&Frame.header) == 2)
			{
				//Right channel. If the stream is monophonic, then right = left
				*Sampler = 0;//MadFixedToSshort(Synth.pcm.samples[1][i]);
			}
			else
				Samplel = Sampler;

			/// Here we put the other part of the sample in the output buffer.
			int x = req - BytesOut;
			memcpy(buf+BytesOut, tbuf, x);
			memcpy(OUTPUT_BUFFER+BytesInOutput, tbuf+x, 4-x);
			BytesOut += x;
			BytesInOutput += (4 - x);
			++i;
		}

		for (;i < Synth.pcm.length;++i) {

			/// Put all that could not fit in buf, in the output buffer.

			signed short* Samplel = (signed short*) (OUTPUT_BUFFER + BytesInOutput);
			signed short* Sampler = (signed short*) (OUTPUT_BUFFER + BytesInOutput + 2);
			// Left Channel
			*Samplel = MadFixedToSshort(Synth.pcm.samples[0][i]);

			if (MAD_NCHANNELS(&Frame.header) == 2)
			{
				//Right channel. If the stream is monophonic, then right = left
				*Sampler = 0;//MadFixedToSshort(Synth.pcm.samples[1][i]);
			}
			else
				Samplel = Sampler;

			BytesInOutput += 4;
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
		return(SHRT_MAX);
	if(Fixed<=-MAD_F_ONE)
		return(-SHRT_MAX);

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}
