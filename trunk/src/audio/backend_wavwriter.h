#ifndef BACKEND_WAVWRITER_H
#define BACKEND_WAVWRITER_H
#include "backend_interface.h"
#include <boost/thread.hpp>

class WAVWriterBackend : public IBackend {
	public:
		WAVWriterBackend(AudioController* dec);
		virtual ~WAVWriterBackend();
	private:
		AudioController* decoder;
		FILE* f;
		bool done;
		boost::thread* outputter_thread;
		void outputter();
		void start_output();
		void stop_output();
};

#endif//BACKEND_LIBAO_H
