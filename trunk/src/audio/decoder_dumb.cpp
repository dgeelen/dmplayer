#include "decoder_dumb.h"

#include "../error-handling.h"
#define BUFSIZE 8192

int DUMBDecoder::DUMBFS_skip(void* f, long n) {
	return static_cast<DUMBDecoder*>(f)->DUMBFS_skip(n);
}

int DUMBDecoder::DUMBFS_getc(void* f) {
	return static_cast<DUMBDecoder*>(f)->DUMBFS_getc();
}

long DUMBDecoder::DUMBFS_getnc(char* ptr, long n, void* f) {
	return static_cast<DUMBDecoder*>(f)->DUMBFS_getnc(ptr, n);
}

int DUMBDecoder::DUMBFS_skip(long n) {
	boost::circular_buffer<uint8>::iterator begin, end;
		while(n>0) {
		begin = buffer.begin();
		if(buffer.size() >= n) {
			end = begin + n;
			n = 0;
		}
		else {
			end = buffer.end();
			n -= buffer.size();
		}

		buffer.rerase(begin, end);
		if(buffer.size() == 0) fill_buffer();
		if((buffer.size() == 0) && (n != 0)) { // EOF reached while skipping
			return -1;
		}
	}
	return 0;
}

int DUMBDecoder::DUMBFS_getc() {
	if(buffer.size() == 0) fill_buffer();
	if(buffer.size() == 0) return -1;
	int ret = buffer[0];
	buffer.rerase(buffer.begin());
	return ret;
}

long DUMBDecoder::DUMBFS_getnc(char* ptr, long n) {
	long i = 0;
	boost::circular_buffer<uint8>::iterator b = buffer.begin();
	for(; n>0; ++i, --n, ++b) {
		if(b == buffer.end()) {
			buffer.clear();
			fill_buffer();
			if(buffer.size() == 0) {
				return i;
			}
			b = buffer.begin();
		}
		ptr[i] = *b;
	}
	buffer.rerase(buffer.begin(), b);
	return i;
}

DUMBDecoder::DUMBDecoder(IDataSourceRef ds_, DUMB_TYPE filetype)
	: IDecoder(AudioFormat())
	, datasource(ds_)
{
	buffer = boost::circular_buffer<uint8>(BUFSIZE);

	datasource->reset();

	dfs.open = NULL;
	dfs.skip = &DUMBDecoder::DUMBFS_skip;
	dfs.getc = &DUMBDecoder::DUMBFS_getc;
	dfs.getnc = &DUMBDecoder::DUMBFS_getnc;
	dfs.close = NULL;
	dumbfile = dumbfile_open_ex((void*)this, &dfs);

	if(!dumbfile) {
		throw Exception("Could not open LibDUMB");
	}

	switch(filetype) {
		case DUMB_TYPE_IT: {
			duh = dumb_read_it_quick(dumbfile);
		}; break;
		case DUMB_TYPE_XM: {
			duh = dumb_read_xm_quick(dumbfile);
		}; break;
		case DUMB_TYPE_S3M: {
			duh = dumb_read_s3m_quick(dumbfile);
		}; break;
		case DUMB_TYPE_MOD: {
			duh = dumb_read_mod_quick(dumbfile);
		}; break;
		default: {
			duh = NULL;
		}
	}

	if(!duh) {
		throw Exception("Error while intializing DUH");
	}

	audioformat.Float = false;
	audioformat.Channels = 2;
	audioformat.BitsPerSample = 16;
	audioformat.SignedSample = true;
	audioformat.LittleEndian = true;
	audioformat.SampleRate = 44100;

	duh_sigrenderer = duh_start_sigrenderer(duh, 0, audioformat.Channels, 0);
	if(!duh_sigrenderer) {
		throw Exception("Error initializing DUH Renderer");
	}

	dumb_it_set_loop_callback(duh_get_it_sigrenderer(duh_sigrenderer), &DUMBDecoder::loop_callback, this);
	n_loops = 0;
	duration = 0;
	is_exhausted = false;
}

DUMBDecoder::~DUMBDecoder() {
	duh_end_sigrenderer(duh_sigrenderer);
	dumb_exit();
}

void DUMBDecoder::fill_buffer() {
	std::vector<uint8> b(BUFSIZE);
	while(buffer.size() != BUFSIZE) {
		int read = datasource->getData( &b[0], BUFSIZE - buffer.size());
		if (read == 0 && datasource->exhausted()) break;
		buffer.insert(buffer.end(), b.begin(), b.begin() + read);
	}
}

IDecoderRef DUMBDecoder::tryDecode(IDataSourceRef ds) {
	const int min_input_size = 2048;
	boost::shared_array<char> b = boost::shared_array<char>(new char[min_input_size]);
	memset(&b[0], 0, min_input_size);
	ds->reset();
	ds->getData((uint8*)&b[0], min_input_size);

	DUMB_TYPE type = DUMB_TYPE_INVALID;

	// Match Impulse Tracker module sound data (audio/x-it), taken from File Magic:
	// 0	string		IMPM		Impulse Tracker module sound data -
	// >4	string		>\0		"%s"
	// >40	leshort		!0		compatible w/ITv%x
	// >42	leshort		!0		created w/ITv%x
	if(strncmp(&b[0], "IMPM", 4) == 0) {
		dcerr("Detected Impulse Tracker module sound data");
		type = DUMB_TYPE_IT;
	}

	// Match Fasttracker II module sound data, taken from File Magic:
	// 0	string	Extended\ Module: Fasttracker II module sound data
	// >17	string	>\0		Title: "%s"
	if(strncmp(&b[0], "Extended Module: ", 17) == 0) {
		dcerr("Detected Fasttracker II module sound data");
		type = DUMB_TYPE_XM;
	}

	// Match ScreamTracker III Module sound data, taken from File Magic:
	// 0x2c	string		SCRM		ScreamTracker III Module sound data
	// >0	string		>\0		Title: "%s"
	if(strncmp(&b[0x2c], "SCRM", 4) == 0) {
		dcerr("ScreamTracker III Module sound data");
		type = DUMB_TYPE_S3M;
	}

	// Match Protracker module sound data, taken from File Magic:
	// 1080	string	M.K.		4-channel Protracker module sound data
	// >0	string	>\0		Title: "%s"
	// 1080	string	M!K!		4-channel Protracker module sound data
	// >0	string	>\0		Title: "%s"
	if((strncmp(&b[1080], "M.K.", 4) == 0) || (strncmp(&b[1080], "M!K!", 4) == 0)) {
		dcerr("Protracker module sound data");
		type = DUMB_TYPE_MOD;
	}

	if(type != DUMB_TYPE_INVALID) {
		try {
			return IDecoderRef(new DUMBDecoder(ds, type));
		}
		catch( Exception e ) {
			std::cout << "DUMBDecoder: Possible bug in LibDUMB, file will not play!" << std::endl;
			return IDecoderRef();
		}
	}
	else {
		dcerr("Could not match data format");
		return IDecoderRef();
	}
}

int DUMBDecoder::loop_callback(void* data) {
	return static_cast<DUMBDecoder*>(data)->loop_callback();
}

int DUMBDecoder::loop_callback() {
	n_loops++;
	dcerr("Loop! count="<<n_loops);
	long pos = duh_sigrenderer_get_position(duh_sigrenderer);
	if(!duration) duration = pos;
	if(pos + duration <= 65536 * (3*60 + 20) + (duration >> 3)) // FIXME: (duration >> 3): Is this what we want?
		return 0;
	else
		return -1;
}

bool DUMBDecoder::exhausted() {
	return is_exhausted;
}

uint32 DUMBDecoder::getData(unsigned char* buf, unsigned long len) {
	uint32 bytes_per_frame = audioformat.Channels * sizeof(int16);
	uint32 nframes         = len / bytes_per_frame;
	float delta = 65536.0f / (float)audioformat.SampleRate;
	long result = duh_render(duh_sigrenderer, audioformat.BitsPerSample, !audioformat.SignedSample, 1.0f, delta, nframes, buf);
	is_exhausted = (result != nframes);
	return result * audioformat.Channels * sizeof(int16);
}