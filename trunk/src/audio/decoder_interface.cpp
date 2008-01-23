#include "decoder_interface.h"

#include "../error-handling.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>

std::vector<boost::function<IDecoderRef (IDataSourceRef)> > decoderlist;

#define UNIQUE_NAMESPACE_HELPER2(x, y) x ## y
#define UNIQUE_NAMESPACE_HELPER1(a, b) UNIQUE_NAMESPACE_HELPER2(a,b)

#define UNIQUE_NAMESPACE UNIQUE_NAMESPACE_HELPER1(ns, __LINE__)

#define REGISTER_DECODER_FUNCTION(func) namespace UNIQUE_NAMESPACE { static int x = (decoderlist.push_back(func), 10); }
#define REGISTER_DECODER_CLASS(cls) REGISTER_DECODER_FUNCTION(boost::bind(& cls ::tryDecode, boost::shared_ptr<cls>(new cls()), _1) )

#include <decoder_linker.inc>

#undef UNIQUE_NAMESPACE_HELPER2
#undef UNIQUE_NAMESPACE_HELPER1
#undef UNIQUE_NAMESPACE
#undef REGISTER_DECODER_FUNCTION
#undef REGISTER_DECODER_CLASS

IDecoderRef IDecoder::findDecoder(IDataSourceRef ds)
{
	for (unsigned int i = 0; i < decoderlist.size(); ++i) {
		IDecoderRef decoder = decoderlist[i](ds);
		if (decoder) {
			dcerr("Found a decoder; decoder #"<<i);
			return decoder;
		}
	}
	return IDecoderRef(); // NULL reference
}
