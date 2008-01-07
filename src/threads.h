#ifndef THREADS_H
	#define THREADS_H
	#include "cross-platform.h"


	template < typename T >
	THREAD spawnThread(int (WINAPI * start_routine)(T*), T *args);

// Note: we could use something like this to pass a member function,
// but it becomes a real p.i.t.a, and is generally not recommended.
// (http://www.parashift.com/c++-faq-lite/pointers-to-members.html#faq-33.2)
//
// 	template < class C, typename T >
// 	THREAD spawnThread(int (WINAPI C::* start_routine)(T*), T *args);


	//Since the 'export' keyword is not supported by GCC we do it this way
	#define NOTHING_TO_SEE_HERE_PEOPLE
	#include "threads.cpp"
	#undef NOTHING_TO_SEE_HERE_PEOPLE
#endif //THREADS_H
