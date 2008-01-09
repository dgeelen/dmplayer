#ifndef ERROR_HANDLING_H
	#define ERROR_HANDLING_H
	#include <iostream>
	#include <boost/function.hpp>
	/* DEBUG #define's */
	#ifdef DEBUG
		#define dcerr(x) std::cerr << x << "\n"
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
