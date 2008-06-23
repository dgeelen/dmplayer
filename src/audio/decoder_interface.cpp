#include "decoder_interface.h"
#include "../error-handling.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/utility/result_of.hpp>

namespace { namespace ns_reghelper {
	std::vector<boost::function<IDecoderRef (IDataSourceRef)> > decoderlist;
	std::vector<std::string> decodernamelist; //#ifdef debug etc...

/*	template<typename T, class F>
	int doregister(F f);
*/
	template<typename T>
	int doregister(IDecoderRef f(IDataSourceRef), std::string name)
	{
		decoderlist.push_back(
			boost::bind(& T::tryDecode,
				_1
			)
		);
		decodernamelist.push_back(name);
		return 1;
	}

	template<typename T>
	int doregister(IDecoderRef (T::*f)(IDataSourceRef), std::string name)
	{
		decoderlist.push_back(
			boost::bind(& T::tryDecode,
				boost::shared_ptr<T>(new T()),
				_1
			)
		);
		decodernamelist.push_back(name);
		return 2;
	}

	//template<typename T, class F>
	//static int doregister(F f)
	//{
	//	assert(false);
	//}
} } //namespace { namespace nshelper {

#define REGISTER_DECODER_CLASS(cls) namespace { \
	namespace ns_cls_ ## cls { \
		int x = ns_reghelper::doregister<cls>(&cls ::tryDecode, #cls); \
		} \
 }

#include <decoder_linker.inc>

#undef REGISTER_DECODER_CLASS

IDecoderRef IDecoder::findDecoder(IDataSourceRef ds)
{
	IDecoderRef decoder;
	for (unsigned int i = 0; i < ns_reghelper::decoderlist.size(); ++i) {
		ds->reset();
		decoder = ns_reghelper::decoderlist[i](ds);
		if (decoder) {
			dcerr("Found a decoder: " << ns_reghelper::decodernamelist[i]);
			break;
		}
	}
	return decoder;
}
