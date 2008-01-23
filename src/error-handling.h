#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <iostream>
#include <stdexcept>
#include <boost/function.hpp>

class ErrorHandler {
	public:
		ErrorHandler(boost::function<void()> f);
		void operator()();
	private:
		boost::function<void()> f;
};

/* DEBUG #define's */
#ifdef DEBUG
	#include <string>
	#include <iomanip>
	namespace DCERR {
		static std::string file_basename(std::string s) {
			int i = s.find_last_of("/\\", s.size());
			s = s.substr( i + 1, s.size() - i - 1);
			return s;
		}
	}
	#define dcerr(x) std::cerr << std::setiosflags(std::ios::right) \
	                           << std::setw(28) << DCERR::file_basename(__FILE__) \
	                           << std::setw(5) << __LINE__ << ' ' \
	                           << std::setw(28) << __FUNCTION__ << "(): " \
	                           << std::setiosflags(std::ios::left) \
	                           << x \
	                           << '\n'
#else
	#define dcerr(x)
#endif

#endif //ERROR_HANDLING_H
