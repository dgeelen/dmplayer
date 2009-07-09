#ifndef DECODER_DUMB_H
#define DECODER_DUMB_H

#include "decoder_interface.h"
#include <dumb.h>
#include <boost/shared_array.hpp>
#include <boost/circular_buffer.hpp>

class DUMBDecoder: public IDecoder {
	public:
		~DUMBDecoder();

		/* IDecoder Interface */
		bool exhausted();
		static IDecoderRef tryDecode(IDataSourceRef ds);
		uint32 getData(uint8* buf, uint32 len);

	private:
		enum DUMB_TYPE {
			DUMB_TYPE_IT = 0,
			DUMB_TYPE_XM,
			DUMB_TYPE_S3M,
			DUMB_TYPE_MOD,
			DUMB_TYPE_INVALID
		};
		DUMBDecoder(IDataSourceRef ds, DUMB_TYPE filetype);
		void fill_buffer();
		IDataSourceRef datasource;
		boost::circular_buffer<uint8> buffer;
		bool is_exhausted;

		// Used to prevent music from looping for ever
		static int loop_callback(void* data);
		int loop_callback();
		int n_loops;
		long duration;

		/* The following functions are used to implement a DUMBFILE system */
		       int    DUMBFS_skip (         long n           );
		       int    DUMBFS_getc (                          );
		       long   DUMBFS_getnc(char* ptr, long n         );
		static int    DUMBFS_skip (void* f, long n           );
		static int    DUMBFS_getc (void* f                   );
		static long   DUMBFS_getnc(char* ptr, long n, void* f);
		struct DUMBFILE_SYSTEM dfs;

		DUMBFILE*        dumbfile;
		DUH*             duh;
		DUH_SIGRENDERER* duh_sigrenderer;
};

#endif//DECODER_DUMB_H
