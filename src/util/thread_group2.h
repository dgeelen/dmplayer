#ifndef THREAD_GROUP2_H
#define THREAD_GROUP2_H

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <iterator>
#include "../types.h"

namespace util {
/**
 * thread_group2 is an alternative implementation of boost::thread_group with more intuitive
 * behaviour. It is hoped that at some point in the not-so-distant future boost::thread_group
 * will natively provide the functionality that is implemented here. Therefore thread_group2 is
 * designed as a drop-in replacement for boost::thread_group.
 */
class thread_group2 : private boost::noncopyable {
	public:
		/**
		 * Constructs an empty thread_group2.
		 */
		thread_group2();
		/**
		 * Standard deconstructor.
		 */
		~thread_group2();

		/**
		 * Creates a new thread and adds it to the threadpool managed by
		 * this thread_group2.
		 * @return a pointer to the newly created thread.
		 */
		boost::thread* create_thread(const boost::function0<void>& threadfunc);

		/**
		 * Add an existing thread to the threadpool managed by this thread_group2.
		 * @note the caller gives up management of the thread.
		 */
		void add_thread(boost::thread* thrd);

		/**
		 * Removes a thread from the threadpool managed by this thread_group2.
		 * @note the caller assumes management of the thread
		 */
		void remove_thread(boost::thread* thrd);

		/**
		 * Calls join() on all currently executing threads in the threadpool.
		 * @note no new threads can start until this function returns.
		 */
		void join_all();

		/**
		 * Calls interrupt() on all currently executing threads in the threadpool.
		 */
		void interrupt_all();

		/**
		 * Returns the number of threads currently in the threadpool.
		 */
		int size() const;

		/**
		 * Locks this instance of thread_group2. No new threads can start and
		 * no existing threads can exit until the lock is released.
		 * @return a boost::mutex::scoped_lock.
		 */
		boost::mutex::scoped_lock lock() {
			return boost::mutex::scoped_lock(threads_mutex);
		};
		
		/**
		 * Returns a reference to the i'th thread in the threadpool.
		 * @param pos is a valid index ino the thread pool.
		 * @note valid indices are <i>( i : 0 &leq; i < this->size() )</i>.
		 * @return a a reference to a boost::thread object.
		 */
		boost::thread& operator[](int pos) const { return *threads[pos]; };

		/**
		 * This may not be possible because begin() and end() can not be
		 * reliably determined for any given iterator. Use lock() & operator[]
		 * instead.
		 */
		//// Provide iterator access to the thread objects
		//class iterator : public std::iterator<std::forward_iterator_tag, boost::shared_ptr<boost::thread> > {
		//	public:
		//		iterator() : pos(0) {};
		//		iterator(const thread_group2* threadgroup_,
		//			     long pos_,
		//				 boost::shared_ptr<boost::mutex::scoped_lock> plock_)
		//			: threads(threadgroup_->threads), pos(pos_), plock(plock_)
		//			{
		//				// We only need to lock until threads is initialized,
		//				// perhaps this can be made implicit by ignoring plock_,
		//				// but leave it like this for now.
		//				plock->unlock();
		//			};
		//		boost::shared_ptr<boost::thread> operator*() { return threads[pos]; };
		//		bool operator==(const iterator& i) const { return (i.threads == threads) && (i.pos == pos); };
		//		bool operator!=(const iterator& i) const { return ! (i == (*this)); };
		//		iterator operator++() { ++pos; return *this; };
		//	private:
		//		long pos;
		//		boost::shared_ptr<boost::mutex::scoped_lock> plock;
		//		std::vector<boost::shared_ptr<boost::thread> > threads;
		//}; typedef iterator const_iterator;
		//iterator begin() { return iterator(this,              0, boost::shared_ptr<boost::mutex::scoped_lock>(new boost::mutex::scoped_lock(threads_mutex))); };
		//iterator end() {   return iterator(this, threads.size(), boost::shared_ptr<boost::mutex::scoped_lock>(new boost::mutex::scoped_lock(threads_mutex))); };

	private:
		//bool done;
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