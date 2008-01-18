#ifndef ERROR_HANDLING_H
	#define ERROR_HANDLING_H
	#include <iostream>
	#include <boost/function.hpp>
	/* DEBUG #define's */
	#ifdef DEBUG
		#include <string>
		#include <algorithm>
		#include <sstream>
		namespace DCERR {
		static std::string file_basename(std::string s) {
			int i = s.find_last_of("/", s.size());
			s = s.substr( i + 1, s.size() - i - 1);
			return s;
		}
		static std::stringstream ss;
		}
		#define dcerr_inttostr(x) ((DCERR::ss.str("")),(DCERR::ss<<x),(DCERR::ss.str()))
		#define dcerr_filebasename (std::string(DCERR::file_basename(__FILE__)))
		#define dcerr_filename_part (dcerr_filebasename + std::string( std::max(24 - (int)dcerr_filebasename.size(), 0), ' '))
		#define dcerr_linenumber (dcerr_inttostr(__LINE__))
		#define dcerr_linenumber_part ( std::string(std::max(5 - (int)dcerr_linenumber.size(), 0), '0') + dcerr_linenumber )
		#define dcerr_function (std::string(__FUNCTION__))
		#define dcerr_function_part (dcerr_function + "():" + std::string(std::max(24 - (int)dcerr_function.size(), 1), ' '))
		#define dcerr(x) std::cerr << dcerr_filename_part \
		                           << dcerr_linenumber_part << " " \
		                           << dcerr_function_part \
		                           << x \
		                           << "\n"
	#else
		#define dcerr(x)
	#endif

	class error_handler {
		public:
			error_handler( boost::function<void()> f ) {
				this->f=f;
			};
			void operator()() {
				try {
					f();
				}
				catch(char const* error_msg) {
					dcerr("---<ERROR REPORT>---\n"
							<< error_msg << "\n"
							<< "---</ERROR REPORT>---\n");
				}
				catch(...) {
					dcerr("---<ERROR REPORT>---\n"
							<< "Unknown error ?!\n"
							<< "---</ERROR REPORT>---\n");
				}
			};
		private:
			boost::function<void()> f;
	};
#endif //ERROR_HANDLING_H
