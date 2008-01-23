#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <iostream>
#include <stdexcept>
#include <boost/function.hpp>
//#include <boost/exception.hpp>

class Exception: public std::exception {
	private:
		char* msg;
		int msglen;
		void create(const char*) throw();
	protected:
		void setMessage(const char*);
	public:
		Exception(const char*);
		Exception(const std::string&);
		Exception(const Exception&);
		virtual ~Exception() throw();
		virtual const char* what() const throw();
};

class ReturnValueException: public Exception {
	private:
		int retval;
	public:
		ReturnValueException(int value);
		int getValue() const;
};

/// Create exception class identical to type 'base', but with a new type 'name'
#define DEFINE_EXCEPTION(name, base) \
	class name: public base { \
		public: \
			name()                    : base(std::string(#name) + " : <unspecified>") {}; \
			name(const char* c)       : base(std::string(#name) + " : " + c         ) {}; \
			name(const std::string& s): base(std::string(#name) + " : " + s         ) {}; \
			name(const Exception& e)  : base(std::string(#name) + " : " + e.what()  ) {}; \
			name(const name& e)       : base(e.what()) {};\
	}

DEFINE_EXCEPTION(IOException, Exception);
DEFINE_EXCEPTION(SoundException, IOException);
DEFINE_EXCEPTION(FileException, IOException);
DEFINE_EXCEPTION(NetworkException, IOException);
DEFINE_EXCEPTION(HTTPException, NetworkException);
DEFINE_EXCEPTION(LogicError, Exception);
DEFINE_EXCEPTION(ThreadException, Exception);

class ErrorHandler {
	public:
		ErrorHandler(boost::function<void()> f, bool silent = false);
		void operator()();
	private:
		boost::function<void()> f;
		bool silentreturnexception;
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
