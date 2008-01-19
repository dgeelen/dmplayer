#ifndef ERROR_HANDLING_H
	#define ERROR_HANDLING_H
	#include <iostream>
	#include <boost/function.hpp>
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
		#define dcerr(x) std::cerr << std::setiosflags(std::ios::left) \
		                           << std::setw(28) << DCERR::file_basename(__FILE__) \
		                           << std::setw(5) << __LINE__ << ' ' \
		                           << std::setw(28) << __FUNCTION__ << ' ' \
		                           << std::setiosflags(std::ios::right) \
		                           << x \
		                           << '\n'
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
