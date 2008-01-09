#pragma once

#ifndef CROSS_PLATFORM_H
  #define CROSS_PLATFORM_H
  /* fix for MSVS (C++ 2005 express edition) */
  #if defined(_MSC_VER)
    #define _CRT_SECURE_NO_DEPRECATE
    #define snprintf _snprintf
  #endif

	//FIXME: HAVE_CONFIG_H is a automake thing, use CMake instead
  #ifdef HAVE_CONFIG_H
    #include <config.h>
  #elif defined(__WIN32__) || defined(WIN32)
    // borland/msvs win32 specifics..
    #define HAVE_WINSOCK
    #define HAVE_WSNWLINK_H
  #endif

  #if defined(HAVE_UNISTD_H)
    # include <unistd.h>
  #endif

// TODO: properly figure network stuff out (in conjunction with cmake)
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


	/* general headers */
	#include "types.h"
	#include <string>
	#include <iostream>
	#include <stdio.h>
	#include <assert.h>
	#include <vector>
	#include <queue>
	#include <algorithm>

	/* Crossplatform Thread handling */
	#ifndef __WIN32__
		//<FIXME: This should be detected by cmake, and used as guard instead>
		#define HAVE_PTHREAD_H
		//</FIXME>

		#include <pthread.h>
		#define THREAD pthread_t
		#define THREAD_CREATE_OK 0
		//pthread_exit() takes a void* argument
		#define ThreadExit(x) pthread_exit((void*)(x))

		#undef WINAPI
		#define WINAPI
	#else
		#include <windows.h>
		#define THREAD DWORD
		#define ThreadExit ExitThread
	#endif

//	/* Listing and manipulating filesystem objects (files && directories) */
//	#include <dirent.h>

#endif //CROSS_PLATFORM_H
