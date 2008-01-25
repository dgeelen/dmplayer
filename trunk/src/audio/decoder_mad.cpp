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

MadDecoder::MadDecoder() : IDecoder(AudioFormat())
{
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	BytesInOutput = 0;
	BytesInInput = 0;
	eos = false;
}

MadDecoder::MadDecoder(AudioFormat af, IDataSourceRef ds) : IDecoder(af)
{
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);
	BytesInOutput = 0;
	BytesInInput = 0;
	eos = false;
	datasource = ds;
	datasource->reset();
}

IDecoderRef MadDecoder::tryDecode(IDataSourceRef datasource)
{
	datasource->reset();
	IDecoderRef result;
	BytesInInput = 0;
	while (BytesInInput < INPUT_BUFFER_SIZE && !datasource->exhausted())
		BytesInInput += datasource->getData(input_buffer+BytesInInput, INPUT_BUFFER_SIZE-BytesInInput);

	Stream.next_frame = input_buffer; // make sure while loop is entered at least once

	//return new MadDecoder(datasource);

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
				result = IDecoderRef(new MadDecoder(af, datasource));
			}
		}
		if (Stream.error == MAD_ERROR_BUFLEN)
			break;
	}

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
			eos = datasource->exhausted();
		}

		if (BytesInInput == 0) return BytesOut;
		//Buffer is sent to MAD to decode to a stream
		mad_stream_buffer(&Stream, input_buffer, BytesInInput);
		Stream.error = MAD_ERROR_NONE;

		//Decode the frame here
		if(mad_frame_decode(&Frame, &Stream)){
			if (!MAD_RECOVERABLE(Stream.error)) {
				dcerr("Unrecoverable error");
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
			for (int j = 0; j < audioformat.Channels; ++j) {
				*((signed short*)(buf+BytesOut)) = MadFixedToSshort(Synth.pcm.samples[j][i]);
				BytesOut += 2;
			}
		}

		for (;i < Synth.pcm.length;++i) {
			for (int j = 0; j < audioformat.Channels; ++j) {
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
		return(SHRT_MAX);
	if(Fixed<=-MAD_F_ONE)
		return(-SHRT_MAX);

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}