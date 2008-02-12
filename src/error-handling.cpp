#include "error-handling.h"

#include <iomanip>

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
 * error logging stuff
 */
namespace lognamespace {
	boost::signal<void(const std::string&, const std::string&, const std::string&, int)> logsignal;
}

namespace {
	using namespace lognamespace;

	std::string file_basename(std::string s) {
		int i = s.find_last_of("/\\", s.size());
		s = s.substr( i + 1, s.size() - i - 1);
		return s;
	}

	void logfunc1(const std::string& msg, const std::string& file, const std::string& func, int line)
	{
		stringstream ss;
		ss << std::setiosflags(std::ios::right);
		ss << std::setw(28) << file_basename(file);
		ss << std::setw(5) << line << ' ';
		ss << std::setw(28) << func << "(): ";
		ss << std::setiosflags(std::ios::left);
		ss << msg;
		ss << '\n';
		std::cerr << ss.str();
	}

	void logfunc2(const std::string& msg, const std::string& file, const std::string& func, int line)
	{
		stringstream ss;
		ss << msg << std::endl;
		std::cerr << ss.str();
	}

	int x = (logsignal.connect(&logfunc1), 1);
}
