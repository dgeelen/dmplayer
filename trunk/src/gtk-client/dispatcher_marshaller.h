#ifndef BOOST_PP_IS_ITERATING

#ifndef DISPATCHER_MARSHALLER_H
#define DISPATCHER_MARSHALLER_H

#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <glibmm/dispatcher.h>

#ifndef DISPATCHER_MARSHALLER_FUNCTION_MAX_ARITY
#	define DISPATCHER_MARSHALLER_FUNCTION_MAX_ARITY 10
#endif

class DispatcherMarshaller : public Glib::Dispatcher {
	private:
		typedef boost::function<void(void)> function_type;

		boost::mutex               queue_mutex;
		std::vector<function_type> queue;
		// This is called by the Dispatcher object to run in the thread
		// DispatcherMarshaller belongs to.
		void queue_runfunction() {
			boost::mutex::scoped_lock lock(queue_mutex);
			BOOST_FOREACH(function_type f, queue) {
				f();
			}
			queue.clear();
		}

		template <typename TCallee>
		class MarshallFunctionObject {
			private:
				DispatcherMarshaller* marshaller;
				TCallee callee;
			public:
				MarshallFunctionObject(DispatcherMarshaller* marshaller_, const TCallee& callee_)
					: marshaller(marshaller_), callee(callee_) {};
				#define BOOST_PP_ITERATION_LIMITS (0, DISPATCHER_MARSHALLER_FUNCTION_MAX_ARITY)
				#define BOOST_PP_FILENAME_1 "dispatcher_marshaller.h" // this file
				#include BOOST_PP_ITERATE()
				typedef void result_type;
		};

	public:
			DispatcherMarshaller() : Glib::Dispatcher() {
				connect(boost::bind(&DispatcherMarshaller::queue_runfunction, this));
			};

			void enqueue(function_type f) {
				boost::mutex::scoped_lock lock(queue_mutex);
				queue.push_back(f);
			};

			void invoke(function_type f) {
				boost::mutex::scoped_lock lock(queue_mutex);
				queue.push_back(f);
				(*this)(); // Notification of dispatcher (which will invoke queue_runfunction)
			};

			template <typename TCallee>
			MarshallFunctionObject<TCallee> wrap(const TCallee& callee_) {
				return MarshallFunctionObject<TCallee>(this, callee_);
			};
};

#endif //DISPATCHER_MARSHALLER_H

#else //BOOST_PP_IS_ITERATING

#define n BOOST_PP_ITERATION()

#if n > 0
template <BOOST_PP_ENUM_PARAMS(n, class T)>
#endif
void operator()(BOOST_PP_ENUM_BINARY_PARAMS(n, const T, &t)) // Part of MarshallFunctionObject
{
	marshaller->invoke(boost::bind(callee
		BOOST_PP_COMMA_IF(n)
		BOOST_PP_ENUM_PARAMS(n,t)
	));
}
#undef n
#endif //BOOST_PP_IS_ITERATING
