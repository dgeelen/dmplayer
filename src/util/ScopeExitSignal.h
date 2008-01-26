#ifndef SCOPE_EXIT_SIGNAL_H
#define SCOPE_EXIT_SIGNAL_H

#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>

// use this class to register function to call when
// the ScopeExitSignal goes out of scope
// added functions are called in LIFO order
class ScopeExitSignal : boost::noncopyable {
	private:
		typedef boost::signal<void()> SigType;
		SigType sig;
	public:
		ScopeExitSignal();
		~ScopeExitSignal();
		void add(const SigType::slot_type &f);
};

#endif//SCOPE_EXIT_SIGNAL_H
