#ifndef THREAD_GROUP2_H
#define THREAD_GROUP2_H

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <iterator>
#include "../types.h"

/**
 * thread_group2 is an alternative implementation of boost::thread_group with more intuitive
 * behaviour. It is hoped that at some point in the not-so-distant future boost::thread_group
 * will natively provide the functionality that is implemented here. Therefore thread_group2 is
 * designed as a drop-in replacement for boost::thread_group.
 */
namespace util {
class thread_group2 : private boost::noncopyable {
	public:
		thread_group2();
		~thread_group2();

		boost::thread* create_thread(const boost::function0<void>& threadfunc);
		void add_thread(boost::thread* thrd);
		void remove_thread(boost::thread* thrd);
		void join_all();
		void interrupt_all();
		int size() const;

		// Provide iterator access to the thread objects
		class iterator : public std::iterator<std::forward_iterator_tag, boost::thread> {
			public:
				iterator() : threadgroup(NULL), pos(0) {};
				iterator(const thread_group2* threadgroup_, long pos_) : threadgroup(threadgroup_), pos(pos_) {};
				boost::thread& operator*() { return *(threadgroup->threads[pos]); };
				bool operator==(const iterator& i) const { return (i.threadgroup == threadgroup) && (i.pos == pos); };
				iterator operator++() { ++pos; return *this; };
			private:
				long pos;
				const class thread_group2* threadgroup;
		}; typedef iterator const_iterator;
		iterator begin() { return iterator(this, 0); };
		iterator end() { return const_iterator(this, threads.size()); };
		const_iterator begin() const { return const_iterator(this, 0); };
		const_iterator end() const { return const_iterator(this, threads.size()); };

		const boost::thread& operator[](int pos) const { return *threads[pos]; };
	private:
		bool done;
		boost::mutex thread_creation_mutex;
		boost::mutex threads_mutex;
		std::vector<boost::shared_ptr<boost::thread> > threads;
// 		boost::shared_ptr<boost::thread> thread_manager_thread;
// 		boost::condition thread_manager_condition;
// 		void thread_manager();
		void thread_runner(const boost::function0<void>& threadfunc);
};
}
#endif // THREAD_GROUP2_H