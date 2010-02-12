#ifndef BACKEND_PORTAUDIO_H
#define BACKEND_PORTAUDIO_H

#include "backend_interface.h"
#include "audio_controller.h"
#include "portaudio.h" // NOTE: *MUST* be portaudio V19

#include "../util/StrFormat.h"

#include <boost/thread.hpp>

// Define this to get output on stderr indicating buffer status
//#define PRINT_BUFSIZE

// TODO: move this to its own file

// Normally data is requested in blocks of size NORMAL_CHUNKSIZE
// But when buffer is less than FASTFILL_PERCENTAGE filled, blocks of size FASTFILL_CHUNKSIZE are used instead
// When buffer is more than WAITFILL_PERCENTAGE filled, we just idle WAITFILL_SLEEPMS ms before trying again
template<typename T, size_t NORMAL_CHUNKSIZE = 64*1024, size_t FASTFILL_PERCENTAGE = 10, size_t FASTFILL_CHUNKSIZE = 2048*8, size_t WAITFILL_PERCENTAGE = 90, size_t WAITFILL_SLEEPMS = 10>
class circular_buffer {
	private:
		boost::mutex cb_mutex;
		std::vector<uint8> cb_buf;
		size_t cb_rpos; // Read position, only modified by read() calls
		size_t cb_wpos; // Write position, only modified by prepare_write() calls
		size_t cb_data; // Amount of data in buffer, modified (with locking) by both read() and prepare_write() calls
		size_t cb_size; // Total capacity of buffer, never modified

	public:
		/**
		 *  Constructs a circular buffen with given capacity
		 */
		circular_buffer(size_t capacity = 1)
		{
			reset(capacity);
		}

		/**
		 *  Clears the buffer and resizes to the given capacity
		 */
		void reset(size_t capacity)
		{
			cb_buf.resize(capacity);
			cb_rpos = 0;
			cb_wpos = 0;
			cb_data = 0;
			cb_size = cb_buf.size();
		}

		/**
		 *  Fills `buf` with up to `count` elements (which are removed from the circular buffer)
		 *  returns the amount of elements filled
		 */
		size_t read(T* buf, size_t count)
		{
			size_t todo = count;

			while (todo > 0) {
				size_t copy = (cb_size - cb_rpos);
				if (todo < copy) copy = todo;
				memcpy(buf, &cb_buf[cb_rpos], copy);
				cb_rpos += copy;
				buf += copy;
				if (cb_rpos == cb_size) cb_rpos = 0;
				todo -= copy;
			}

			size_t failed = count;
			{
				boost::mutex::scoped_lock lock(cb_mutex);
				size_t read = failed;
				if (read > cb_data) read = cb_data;
				failed -= read;
				cb_data -= read;
				#ifdef PRINT_BUFSIZE
					std::cerr << STRFORMAT("Read : %_6.2f  %_9d (%d) - %d", cb_data*100.0/cb_size, cb_data , count-failed , failed) << std::endl;
				#endif
			}

			if (failed > 0) {
				if (cb_rpos < failed)
					cb_rpos += cb_size;
				cb_rpos -= failed;
			}

			return (count - failed);

		}

		/**
		 *  Prepares a buffer for writing data
		 *
		 *  @param buf [out] Will contain a pointer to the start of the prepared buffer
		 *  @param count [out] Will contain the amount of elements that can we written to the prepared buffer
		 *
		 *  After data is written to the buffer call commit_write to commit os
		 */
		void write_prepare(T*& buf, size_t& count) {
			write(buf, count, 0);
		}

		/**
		 *  Commits data written to previously prepared buffer
		 *  This makes it available to the read() operation
		 *
		 *  @param written Amount of data written to the buffer
		 */
		void write_commit(size_t written) {
			write(NULL, 0, written);
		}

		/**
		 *  Commit previously written data and prepare a new buffer
		 *
		 *  The call write(buf, count, written) is equivalent with
		 *     commit_write(written);
		 *     prepare_write(buf, count);
		 *  But uses only one lock operation instead of two
		 */
		void write(T*& buf, size_t& count, size_t written)
		{
			cb_wpos += written;
			if (cb_wpos == cb_size) cb_wpos = 0;
			buf = &cb_buf[cb_wpos];

			{
				boost::mutex::scoped_lock lock(cb_mutex);
				cb_data += written;
				#ifdef PRINT_BUFSIZE
					std::cerr << STRFORMAT("Write: %_6.2f  %_9d (%d)", cb_data*100.0/cb_size, cb_data , written) << std::endl;
				#endif

				while (cb_data * 100 >= cb_size * WAITFILL_PERCENTAGE) {
					cb_mutex.unlock();
					boost::this_thread::sleep(boost::posix_time::milliseconds(WAITFILL_SLEEPMS)); 
					cb_mutex.lock();
				}
				count = cb_size - cb_data;

				if (cb_data * 100 < cb_size * FASTFILL_PERCENTAGE) {
					if (count > FASTFILL_CHUNKSIZE) count = FASTFILL_CHUNKSIZE;
				} else {
					if (count > NORMAL_CHUNKSIZE) count = NORMAL_CHUNKSIZE;
				}
			}

			if (count > cb_size - cb_wpos) count = cb_size - cb_wpos;
		}
};

class PortAudioBackend : public IBackend {
	private:
		static int pa_callback(const void *inputBuffer, void *outputBuffer,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData );

		int data_callback(uint8* out, unsigned long byteCount);

		PaStream *stream;
		AudioController* dec;

		circular_buffer<uint8> cbuf;

		void decodeloop();

		boost::thread decodethread;
	public:
		PortAudioBackend(AudioController* _dec);
		virtual ~PortAudioBackend();
		void start_output();
		void stop_output();
};

#endif//BACKEND_PORTAUDIO_H
