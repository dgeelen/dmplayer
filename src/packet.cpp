#include "packet.h"
using namespace std;

#include "error-handling.h"
#include "util/StrFormat.h"
#include <boost/serialization/shared_ptr.hpp>

// NOTE: I'm not sure why it works, but I think these BOOST_CLASS_EXPORTs need to be here...
BOOST_CLASS_EXPORT(message_connect);
BOOST_CLASS_EXPORT(message_accept);
BOOST_CLASS_EXPORT(message_vote);
BOOST_CLASS_EXPORT(message_disconnect);
BOOST_CLASS_EXPORT(message_playlist_update);
BOOST_CLASS_EXPORT(message_query_trackdb);
BOOST_CLASS_EXPORT(message_query_trackdb_result);
BOOST_CLASS_EXPORT(message_request_file);
BOOST_CLASS_EXPORT(message_request_file_result);

void operator<<(tcp_socket& sock, const messagecref msg)
{
	#ifdef NETWORK_CORE_USE_LOCKS
	boost::mutex::scoped_lock lock(sock.send_mutex);
	#endif
	stringstream ss;
	boost::archive::binary_oarchive oa(ss);
	oa << msg;
	long l = htonl((long)ss.str().size() + 1);
	sock.send((const uint8*)(&l), sizeof(long));
	sock.send((const uint8*)ss.str().c_str(), ss.str().size() +1);
}

void operator>>(tcp_socket& sock,       messageref& msg)
{
	#ifdef NETWORK_CORE_USE_LOCKS
	boost::mutex::scoped_lock lock(sock.recv_mutex);
	#endif
	try {
		long l;
		int rnum;
		rnum = sock.receive((uint8*)&l, sizeof(long));
		if (rnum != sizeof(long))
			throw NetworkException("receive failed");
		l = ntohl(l);
		boost::shared_array<uint8> a = boost::shared_array<uint8>(new uint8[l]);
		rnum = sock.receive(a.get(),l);
		if (rnum != l)
			throw NetworkException("receive failed");
		stringstream ss(string((char*)a.get(), (char*)a.get() + l));
		boost::archive::binary_iarchive ia(ss);
		ia >> msg;
	} catch (std::exception& e) {
		msg = messageref(new message_disconnect(STRFORMAT("Exception while reading from socket: %s", e.what())));
	}
}

