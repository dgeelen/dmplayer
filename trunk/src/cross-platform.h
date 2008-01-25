#pragma once

#ifndef CROSS_PLATFORM_H
  #define CROSS_PLATFORM_H
  /* fix for MSVS (C++ 2005 express edition) */
  #if defined(_MSC_VER)
    #define _CRT_SECURE_NO_DEPRECATE
	#define _CRT_SECURE_NO_WARNINGS
	#define _SCL_SECURE_NO_WARNINGS
    #define snprintf _snprintf
  #endif

	//FIXME: HAVE_CONFIG_H is a automake thing, use CMake instead
  #ifdef HAVE_CONFIG_H
    #include <config.h>
  #elif defined(WIN32)
    // borland/msvs win32 specifics..
    #define HAVE_WINSOCK
    #define HAVE_WSNWLINK_H
  #endif

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

		// usleep
		#include <windows.h>
		#define usleep(x) Sleep(x/1000)
	#else
		// for usleep
		#include <unistd.h>
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


	/* general headers */
	#include "types.h"
	#include <string>
	#include <iostream>
	#include <stdio.h>
	#include <assert.h>
	#include <vector>
	#include <queue>
	#include <algorithm>


//	/* Listing and manipulating filesystem objects (files && directories) */
//	#include <dirent.h>

/* Cross-platform Time */
#ifdef __linux__
	#include <sys/time.h>
	uint64 inline get_time_us() {
		struct timeval tv;
		if(gettimeofday(&tv, NULL) == -1) return 0;
		return tv.tv_sec * 1000000 + tv.tv_usec;
	}
#else
	#ifdef WIN32
		#include <time.h>
		uint64 inline get_time_us() {
			return clock() * (1000000 / CLOCKS_PER_SEC);
		}
	#else
		uint64 inline get_time_us() {
			return -1;
		}
	#endif
#endif

#endif //CROSS_PLATFORM_H
