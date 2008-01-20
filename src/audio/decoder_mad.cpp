#include "decoder_mad.h"
#include "mad.h"
#include "../error-handling.h"

MadDecoder::~MadDecoder()
{

}

MadDecoder::MadDecoder()
{

}

IDecoder* MadDecoder::tryDecode(IDataSource* datasource)
{
	dcerr("");
/*	mad_stream Stream;
	mad_frame Frame;
	mad_synth Synth;
*/	INPUT_BUFFER = new char[INPUT_BUFFER_SIZE];
	datasource->read(INPUT_BUFFER, INPUT_BUFFER_SIZE);



	delete INPUT_BUFFER;
	return 0;
}
