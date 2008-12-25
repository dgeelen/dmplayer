#include "decoder_mencoder.h"

#include "../error-handling.h"

#include "../libexecstream/exec-stream.h"
#include "../network/network-core.h"
#include "datasource_resetbuffer.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <sstream>

using namespace std;

#define WRITEBUFSIZE (1024*1024)
#define READBUFSIZE  (1024)

class MencoderDecoderImplementation {
	public:
		MencoderDecoderImplementation(IDataSourceRef ds_, AudioFormat* af_)
		:	ds(ds_), af(af_)
		{
			//ds = ResetBufferDataSourceRef(new ResetBufferDataSource(ds));
			af->BitsPerSample = 16;
			af->Channels = 2;
			af->SampleRate = 44100;
			af->SignedSample = true;
			af->LittleEndian = true;
			constructed = false;
			listensock.listen(ipv4_lookup("127.0.0.1"), 0);
			listenport = listensock.getPortNumber();
			listendone = false;
			conndone = false;
			hasconnected = false;
			servdone = false;
			cansend = false;
			readlen = 0;
			writelen = 0;
			servthread.swap(boost::thread(makeErrorhandler(boost::bind(&MencoderDecoderImplementation::servfunc, this))));

			//string prog = "mencoder";
			string prog = "c:\\temp\\mpmp\\mencoder.exe";
			stringstream args;
			args <<  "-demuxer" << " rawvideo";
			args << " -rawvideo" << " w=1:h=1";
			args << " -audiofile" << " http://127.0.0.1:" << listenport << "/";
			args << " -of" << " rawaudio";
			args << " -oac" << " lavc";
			args << " -lavcopts" << " acodec=pcm_s16le";
			args << " -ovc" << " copy";
			args << " -o" << " -";
			args << " -";
			args << " -really-quiet";

			exec_stream.set_binary_mode(exec_stream_t::s_in | exec_stream_t::s_out);
			exec_stream.start(prog, args.str());
			exec_stream.set_binary_mode(exec_stream_t::s_in | exec_stream_t::s_out);

			exec_stream.in() << std::endl;

			while (!hasconnected && isMencoderRunning()) {};
			if (!isMencoderRunning()) throw std::exception("fail");
			constructed = true;
		}

		~MencoderDecoderImplementation() {
			dcerr("Shutting down");
			servdone = true;
			servthread.join();
			dcerr("Shut down");
		}

		uint32 getData(uint8* buf, uint32 max) {
			uint32 done = 0;
			uint32 read;
			while (done < max) {
				exec_stream.in().write("\n", 1);
				try {
					exec_stream.out().read((char*)buf+done, max-done);
				} catch (...) {
					read = exec_stream.out().gcount();
					done += read;
					return done;
				}
				read = exec_stream.out().gcount();
				done += read;
			}
			return done;
		}

	private:
		AudioFormat* af;
		IDataSourceRef ds;
		boost::thread servthread;
		tcp_listen_socket listensock;
		tcp_socket connsock;
		exec_stream_t exec_stream;
		uint16 listenport;
		bool servdone;
		bool listendone;
		bool conndone;
		bool hasconnected;
		bool constructed;

		map<string, string> httpinfo;
		string replyhdr;
		uint8 readbuf[READBUFSIZE];
		uint32 readlen;
		uint8 writebuf[WRITEBUFSIZE];
		uint32 writelen;
		bool cansend;

		int64 fposbeg, fposend, fposlen, fsize, fposcur;
	private:

		bool getline(string& line)
		{
			uint i;
			for (i = 0; i+1 < readlen && readbuf[i] != '\r' && readbuf[i+1] != '\n'; ++i);
			if (i+1 >= readlen) return false;
			line = string(readbuf, readbuf+i);
			readlen -= i+2;
			memmove(readbuf, readbuf+i+2, readlen);
			return true;
		}

		void servfunc()
		{
			while (!servdone) {
				int listentime = 100;
				if (connsock.getSocketHandle() != INVALID_SOCKET) {
					listentime = 0;
					int sockstat = SELECT_ERROR;
					if (READBUFSIZE-readlen > 0) sockstat |= SELECT_READ;
					if (cansend && !ds->exhausted()) sockstat |= SELECT_WRITE;

					sockstat = doselect(connsock, 100, sockstat);

					if (sockstat & SELECT_READ) {
						uint32 read = connsock.receive(readbuf+readlen, READBUFSIZE-readlen);
						readlen += read;
						if (read == 0) sockstat |= SELECT_ERROR;
					}

					if (sockstat & SELECT_WRITE) {
						if (replyhdr.size() > 0) {
							uint32 write = connsock.send((uint8*)&replyhdr[0], replyhdr.size());
							replyhdr.erase(0, write);
							cout << "sent " << write << " bytes of reply headers" << std::endl;
						} else if (true || 0 > 0) {
							if (writelen < WRITEBUFSIZE/2) {
								uint32 prog = ds->getData(writebuf+writelen, WRITEBUFSIZE-writelen);
								//if (prog == 0) ds.reset();
								//fposcur += prog;
								writelen += prog;
							}
							if (writelen > 0) {
								uint32 send = connsock.send(writebuf, writelen);
								cout << "sent " << send << " bytes of file" << std::endl;
								writelen -= send;
								//fposlen -= send;
								memmove(writebuf, writebuf+send, writelen);
							}
							//cout << fposlen << '\n';
						}

					}

					if (sockstat & SELECT_ERROR)
						connsock.disconnect();

					if (!cansend)
						parseheaders();

				}
				if (doselect(listensock, listentime, SELECT_READ) & SELECT_READ) {
					tcp_socket* newsock = listensock.accept();
					if (!newsock) {
						servdone = true;
					} else {
						hasconnected = true;
						connsock.swap(*newsock);
						conndone = false;
						delete newsock;
						cansend = false;
						readlen = 0;
						writelen = 0;
						httpinfo.clear();
						replyhdr.clear();
					}
				}
			}
		}

		void parseheaders() {
			string line;
			while (getline(line)) {
				uint32 colonpos = line.find(':');
				if (colonpos != string::npos) {
					string lhs = line.substr(0, colonpos);
					string rhs = line.substr(colonpos+1);
					httpinfo[lhs] = rhs;
				} else if (line == "") {
					// entire header has been received, contruct reply headers
					cout << "headers received" << std::endl;
					int retcode = 200;
					if (httpinfo["Range"] != "") {
						string range = httpinfo["Range"].substr(7);
						uint32 lnpos = range.find('-');
						string str_beg = range.substr(0, lnpos);
						string str_end = range.substr(lnpos+1);
						fposbeg = atoi(str_beg.c_str());
						if (str_end != "") throw IOException("can't handle this!");

						fposend = fsize;
						retcode = 206;

						ds->reset();
						uint8 bbuf[1024];
						uint32 seeker = 0;
						while (seeker < fposbeg) {
							uint32 read = fposbeg - seeker;
							if (read > 1024) read = 1024;
							read = ds->getData(bbuf, read);
							seeker += read;
						}

						//file.seekg(fposbeg, ios::beg);
						cout << "partial request! seeked to target position" << std::endl;
					} else {
						fposbeg = 0;
						fposend = fsize;
						ds->reset();
					}
					fposlen = fposend - fposbeg;
					fposcur = fposbeg;
					stringstream ss;
					ss << "ICY " << retcode << " OK" << "\r\n";
					//ss << "Server: httpsend 0.1" << "\r\n";
					//ss << "Accept-Ranges: bytes" << "\r\n";
					//ss << "Content-Type: video/x-msvideo" << "\r\n";
					//ss << "Content-Range: bytes " << fposbeg << '-' << fposend-1 << '/' << fsize << "\r\n";
					//ss << "Content-Range: bytes " << fposbeg << "-/*" << "\r\n";
					//ss << "Content-Length: " << fposlen << "\r\n";
					ss << "\r\n";
					replyhdr = ss.str();
					cansend = true;
					cout << replyhdr;
				}
			}
		}

		bool isMencoderRunning() {
			try {
				int r = exec_stream.exit_code();
			} catch (...) {
				return true;
			}
			return false;
		}
};

IDecoderRef MencoderDecoder::tryDecode(IDataSourceRef ds)
{
	try {
		IDecoderRef ret(new MencoderDecoder(ds));
		return ret;
	}
	catch (Exception& e) {
		VAR_UNUSED(e); // in debug mode
		dcerr("Mencoder tryDecode failed: " << e.what());
		return IDecoderRef();
	}
}

MencoderDecoder::MencoderDecoder(IDataSourceRef ds)
:	impl(new MencoderDecoderImplementation(ds, &audioformat))
{
}

MencoderDecoder::~MencoderDecoder() {
	dcerr("Shut down");
}

uint32 MencoderDecoder::getData(uint8* buf, uint32 max) {
	return impl->getData(buf,max);
}