#ifndef NETWORK_CROSS_PLATFORM_H
#define NETWORK_CROSS_PLATFORM_H
//FIXME: Do we really need 2 cross-platform.h,
//       the main cross-platform.h allready seems
//       to define most of this stuff...

// TODO: properly figure network stuff out (in conjunction with cmake)
// ??: http://www.cmake.org/pipermail/cmake-commits/2007-March/001026.html
#if __WIN32__
	#define WIN32
#endif

#ifdef WIN32
	// defines needed for any windows header we will include
	#define WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	// assume winsock on win32 for now
	#define HAVE_WINSOCK
#endif

#if defined(HAVE_WINSOCK)
	#include <Winsock2.h>
	#include <ws2tcpip.h>

	#define NetGetLastError() WSAGetLastError()
	#define sipx_family sa_family
#else
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h> // for errno in NetGetLastError() macro
	#include <netipx/ipx.h>
	#include <netdb.h>

	#define closesocket(x) ::close(x)
	#define NetGetLastError() errno

	typedef int SOCKET;

	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
	#define NSPROTO_IPX PF_IPX

	#define sa_nodenum sipx_node
	#define sa_socket sipx_port
	#define sa_netnum sipx_network
#endif

#endif//NETWORK_CROSS_PLATFORM_H
