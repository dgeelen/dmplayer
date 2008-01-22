#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "datasource_interface.h"
#include "../types.h"
#include <vector>
#include <boost/function.hpp>

struct AudioFormat{
	int SampleRate;
	int Channels;
	int BitsPerSample;
	bool SignedSample;
	bool LittleEndian;
};

class IDecoder {
	private:
		int buffersize;
		AudioFormat audioformat;

	public:
		IDecoder(AudioFormat af) {audioformat = af;};
		virtual ~IDecoder() {};
		virtual IDecoder* tryDecode(IDataSource*) = 0;
		const AudioFormat getAudioFormat() {return audioformat;};

		void SetBufferSize(int bufsize) {buffersize = bufsize;};
		int GetBufferSize() {return buffersize;};

		virtual uint32 doDecode(uint8* buf, uint32 max, uint32 req = 0) = 0;
};

extern std::vector<boost::function<IDecoder* (IDataSource*)> > decoderlist;

#endif//DECODER_INTERFACE_H
