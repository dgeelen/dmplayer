#include "error-handling.h"

using namespace std;

/*
 *  Implementation of Exception class 
 */

void Exception::create(const char* amsg) throw ()
{
	try {
		msglen = strlen(amsg)+1;
		msg = new char[msglen];
		memcpy(msg, amsg, msglen);
	} catch (...) {
		msglen = -1;
		msg = "INVALID EXCEPTION: error on message construction";
	}
}

void Exception::setMessage(const char* amsg)
{
	if (msg != NULL && msglen != -1)
		delete msg;
	create(amsg);
}

Exception::Exception(const char* amsg)
{
	create(amsg);
}

Exception::Exception(const std::string& astr)
{
	create(astr.c_str());
}

Exception::Exception(const Exception& e)
{
	create(e.what());
}

const char* Exception::what() const throw()
{
	return msg;
}

Exception::~Exception() throw ()
{
	if (msg != NULL && msglen != -1)
		delete msg;

	msg = NULL;
	msglen = -1;
}

/*
 *  Implementation of ReturnValueException class 
 */

ReturnValueException::ReturnValueException(int value)
	: retval(value)
	, Exception("ReturnValueException + TODO value")
{
	// TODO put value in what() message
}

int ReturnValueException::getValue() const
{
	return retval;
}

/*
 *  Implementation of ErrorHandler class 
 */
ErrorHandler::ErrorHandler( boost::function<void()> f, bool silent)
{
	this->f = f;
	silentreturnexception = silent;
};

void ErrorHandler::operator()() {
	#define PRINT_MSG do { \
		cerr << "---<!ERROR REPORT>---\n"; \
		cerr << msg; \
		if (msg[strlen(msg)-1] != '\n') cerr << '\n'; \
		cerr << "---</ERROR REPORT>---\n"; \
	} while(false)

	try {
		f();
	} catch (const ReturnValueException& e) {
		if (!silentreturnexception) {
			const char* msg = e.what();
			PRINT_MSG;
		}
		throw;
	} catch (const exception& e) { // catch by reference do ensure virtual calls work as needed
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
};

