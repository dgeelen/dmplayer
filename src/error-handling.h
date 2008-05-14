#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <iostream>
#include <stdexcept>
#include <boost/function.hpp>
#include <boost/signal.hpp>
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

template<typename T>
class ErrorHandler {
	public:
		typedef T result_type;
		ErrorHandler(boost::function<T()> _f) : f(_f) {};
		T operator()() {
			#define PRINT_MSG do { \
				std::cerr << "---<!ERROR REPORT>---\n"; \
				std::cerr << msg; \
				if (msg[strlen(msg)-1] != '\n') std::cerr << '\n'; \
				std::cerr << "---</ERROR REPORT>---\n"; \
			} while(false)

			try {
				return f(); // *should* be valid C++ even when return type 'T' is 'void'
			} catch (const Exception& e) { // catch by reference to ensure virtual calls work as needed
				const char* msg = e.what();
				PRINT_MSG;
				throw;
			} catch (const std::exception& e) { // catch by reference to ensure virtual calls work as needed
				const char* msg = e.what();
				PRINT_MSG;
				throw;
			} catch (const char* msg) {
				PRINT_MSG;
				throw;
			} catch (...) {
				const char* msg = "UNKNOWN ERROR";
				PRINT_MSG;
				throw;
			}
			#undef PRINT_MSG
		};
	private:
		boost::function<T()> f;
};

template<typename T>
ErrorHandler<typename T::result_type> makeErrorHandler(T f) {
	return ErrorHandler<typename T::result_type>(f);
}

#define VAR_UNUSED(x) do { x; } while (false)
/* DEBUG #define's */
#ifdef DEBUG
	#include <string>
	#include <sstream>

	namespace lognamespace {
		extern boost::signal<void(const std::string&, const std::string&, const std::string&, int)> logsignal;
	}
	#define dcerr(x) do { \
		std::stringstream ss; \
		ss << x; \
		lognamespace::logsignal(ss.str(), __FILE__, __FUNCTION__, __LINE__); \
	} while (false)
#else
	#define dcerr(x) do {} while (false)
#endif

#endif //ERROR_HANDLING_H
