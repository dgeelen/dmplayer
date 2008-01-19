#include "decoder_interface.h"
#include <boost/bind.hpp>

std::vector<boost::function<IDecoder* (IDataSource*)> > decoderlist;

#define UNIQUE_NAMESPACE_HELPER2(x, y) x ## y
#define UNIQUE_NAMESPACE_HELPER1(a, b) UNIQUE_NAMESPACE_HELPER2(a,b)

#define UNIQUE_NAMESPACE UNIQUE_NAMESPACE_HELPER1(ns, __LINE__)

#define REGISTER_DECODER_FUNCTION(func) namespace UNIQUE_NAMESPACE { static int x = (decoderlist.push_back(func), 10); }
#define REGISTER_DECODER_CLASS(cls) REGISTER_DECODER_FUNCTION(boost::bind(& cls ## ::tryDecode, new cls(), _1) )

#include <decoder_linker.inc>

#undef UNIQUE_NAMESPACE_HELPER2
#undef UNIQUE_NAMESPACE_HELPER1
#undef UNIQUE_NAMESPACE
#undef REGISTER_DECODER_FUNCTION
#undef REGISTER_DECODER_CLASS
