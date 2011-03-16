#include "thread_group2.h"
#include <boost/foreach.hpp>


using namespace util;
using namespace std;
using namespace boost::foreach;

thread_group2::thread_group2() {
// 	done = false;
// 	thread_manager_thread = shared_ptr<thread>(new thread(bind(&thread_group2::thread_manager, this)));
}

thread_group2::~thread_group2() {
	join_all();
}

void thread_group2::join_all() {
	boost::mutex::scoped_lock clock(thread_creation_mutex); // disallow creation of new threads
	vector<boost::shared_ptr<boost::thread> > my_threads;
	{
		boost::mutex::scoped_lock lock(public_lock_mutex);
		my_threads = threads;
	}
	BOOST_FOREACH(boost::shared_ptr<boost::thread> pt, my_threads) {
		pt->join();
	}
}

boost::mutex::scoped_lock thread_group2::lock() {
	return boost::mutex::scoped_lock(public_lock_mutex);
};

int thread_group2::size() {
	boost::mutex::scoped_lock c_lock(thread_creation_mutex);
	boost::mutex::scoped_lock e_lock(thread_exit_mutex);
	return threads.size();
}

void thread_group2::thread_runner(const boost::function0<void>& threadfunc) {
	threadfunc();
	{
	boost::mutex::scoped_lock e_lock(thread_exit_mutex);
	boost::mutex::scoped_lock p_lock(public_lock_mutex);
	for(std::vector<boost::shared_ptr<boost::thread> >::iterator i = threads.begin(); i != threads.end() ; ++i) {
		if((*i)->get_id() == boost::this_thread::get_id()) {
			threads.erase(i);
			break;
		}
	}
	} //FIXME: Thread is not *really* exited yet, don't know how bad this is...
}

boost::shared_ptr<boost::thread> thread_group2::create_thread(const boost::function0<void>& threadfunc) {
	boost::mutex::scoped_lock c_lock(thread_creation_mutex);
	boost::mutex::scoped_lock p_lock(public_lock_mutex);
	boost::shared_ptr<boost::thread> t = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&thread_group2::thread_runner, this, threadfunc)));
	threads.push_back(t);
	return t;
}