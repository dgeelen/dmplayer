#include "error-handling.h"

using namespace std;

ErrorHandler::ErrorHandler( boost::function<void()> f )
{
	this->f = f;
};

void ErrorHandler::operator()() {
	try {
		f();
	}
	catch(exception e) {
		cerr << "---<!ERROR REPORT>---\n";
		cerr << e.what() << "\n";
		cerr << "---</ERROR REPORT>---\n";
		throw;
	}
	catch(char const* error_msg) {
		cerr << "---<!ERROR REPORT>---\n";
		cerr << error_msg << "\n";
		cerr << "---</ERROR REPORT>---\n";
		throw;
	}
	catch(...) {
		cerr << "---<!ERROR REPORT>---\n";
		cerr << "UNKNOWN ERROR\n";
		cerr << "---</ERROR REPORT>---\n";
		throw;
	}
};

