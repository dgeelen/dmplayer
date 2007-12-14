#ifndef NETWORK_CROSS_PLATFORM_H
#define NETWORK_CROSS_PLATFORM_H

// TODO: properly figure this is in conjunction with cmake
// ??: http://www.cmake.org/pipermail/cmake-commits/2007-March/001026.html

	#if defined(__WIN32__) || defined(WIN32)
		// borland/msvs win32 specifics..
		#define HAVE_WINSOCK
		//#define HAVE_WSNWLINK_H
	#endif

	#if defined(HAVE_WINSOCK)
		#define WIN32_LEAN_AND_MEAN
		#include <Winsock2.h>
		#include <Wsipx.h>
		#include <ws2tcpip.h>

		#ifdef HAVE_WSNWLINK_H
			# include <wsnwlink.h>
		#endif

		#define NetGetLastError() WSAGetLastError()
		#define sipx_family sa_family
	#else
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <errno.h> // for errno in NetGetLastError() macro

		#include <netipx/ipx.h>

		#define closesocket close
		#define NetGetLastError() errno

		typedef int SOCKET;

		#define INVALID_SOCKET  (SOCKET)(~0)
		#define SOCKET_ERROR            (-1)
		#define NSPROTO_IPX PF_IPX

		#define sa_nodenum sipx_node
		#define sa_socket sipx_port
		#define sa_netnum sipx_network
	#endif
#endif //STDAFX_H
