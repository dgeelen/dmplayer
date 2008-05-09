#include "packet.h"
using namespace std;

// NOTE: I'm not sure why it works, but I think these BOOST_CLASS_EXPORTs need to be here...
BOOST_CLASS_EXPORT(message_connect);
BOOST_CLASS_EXPORT(message_accept);
BOOST_CLASS_EXPORT(message_disconnect);
BOOST_CLASS_EXPORT(message_playlist_update);
