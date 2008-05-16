#ifndef NETWORK_CORE_H
	#define NETWORK_CORE_H
	#include "cross-platform.h"
	#include "error-handling.h"
	#include "packet.h"
	#include <sstream>
	#include <iostream>   /* for std::istream, std::ostream, std::string */
	#include <utility>

	struct ipv4_addr {
		union {
			uint32 full;
			struct { uint8 a,b,c,d; }component;
			uint8  array[4];
		};

		bool operator<(const ipv4_addr& addr) const {
			return full < addr.full;
		}

		bool operator==(const ipv4_addr& addr) const {
			return full == addr.full;
		}

		ipv4_addr() {
			full=0;
		}

		ipv4_addr(uint32 addr) {
			full=addr;
		}
	};

	struct ipv6_addr {
		union {
			struct { uint32 a,b,c,d; }component;
			uint32  array[4];
		};

		ipv6_addr() {
			for(int i=0; i<4; ++i) array[i]=0;
		}
	};

	struct ipv4_socket_addr : std::pair<ipv4_addr, uint16> {
		ipv4_socket_addr(ipv4_addr a, uint16 b) : std::pair<ipv4_addr, uint16>(a,b) {};
		ipv4_socket_addr() {};
		std::string std_str() const;
	};
// 	typedef std::pair<ipv6_addr, uint16> ipv6_socket_addr;

	class tcp_socket {
		public:
			tcp_socket( );
			tcp_socket( SOCKET s, ipv4_socket_addr addr );
			tcp_socket( const ipv4_addr addr, const uint16 port );
			tcp_socket( const ipv6_addr addr, const uint16 port );
			void connect( const ipv4_addr addr, const uint16 port );
			void connect( const ipv6_addr addr, const uint16 port );
			bool bind( const ipv4_addr addr, const uint16 port );
			bool bind( const ipv6_addr addr, const uint16 port );
			ipv4_socket_addr get_ipv4_socket_addr();
			uint32 send( const uint8* buf, const uint32 len );
			void operator<<(const messagecref msg);
			void operator>>(      messageref& msg);
			uint32 receive( const uint8* buf, const uint32 len );
			void disconnect();
			SOCKET getSocketHandle() { return sock; };
		private:
			SOCKET sock;
			ipv4_socket_addr peer;
	};
	typedef boost::shared_ptr<tcp_socket> tcp_socket_ref;

	class tcp_listen_socket {
		public:
			tcp_listen_socket();
			tcp_listen_socket(const ipv4_addr addr, const uint16 portnumber);
			tcp_listen_socket(const ipv6_addr addr, const uint16 portnumber);
			tcp_socket* accept();
			SOCKET getSocketHandle() { return sock; };
		private:
			SOCKET sock;
	};

	class udp_socket {
		public:
			udp_socket( );
			udp_socket( const ipv4_addr, const uint16 port );
			udp_socket( const ipv6_addr, const uint16 port );
			bool bind( const ipv4_addr addr, const uint16 port );
			bool bind( const ipv6_addr addr, const uint16 port );

			uint32 send( const ipv4_addr dest_addr, const uint16 dest_port, const uint8* buf, const uint32 len );
			uint32 send( const ipv6_addr dest_addr, const uint16 dest_port, const uint8* buf, const uint32 len );
			uint32 send( const ipv4_addr dest_addr, const uint16 dest_port, packet& p );
			uint32 receive( ipv4_addr* from_addr, uint16* from_port, const uint8* buf, const uint32 len );
			uint32 receive( ipv6_addr* from_addr, uint16* from_port, const uint8* buf, const uint32 len );
			uint32 receive( ipv4_addr* from_addr, uint16* from_port, packet& p );

			void close();
			SOCKET getSocketHandle() { return sock; };
		private:
			SOCKET sock;
	};

	enum {
		SELECT_READ  = 1 << 0,
		SELECT_WRITE = 1 << 1,
		SELECT_ERROR = 1 << 2,
	};

	template <typename T>
	uint32 doselect(T& sock, uint32 timeout = 1000, uint32 mask = SELECT_READ|SELECT_WRITE|SELECT_ERROR) {
		SOCKET handle = sock.getSocketHandle();
		fd_set rset;
		fd_set wset;
		fd_set eset;
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_ZERO(&eset);
		if (mask & SELECT_READ)
			FD_SET(handle, &rset);
		if (mask & SELECT_WRITE)
			FD_SET(handle, &wset);
		if (mask & SELECT_ERROR)
			FD_SET(handle, &eset);

		timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = 1000 * (timeout % 1000);

		uint32 retval = 0;
		int sel = select(handle+1, &rset, &wset, &eset, &tv);
		if (sel > 0) {
			if (FD_ISSET(handle, &rset)) retval |= SELECT_READ;
			if (FD_ISSET(handle, &wset)) retval |= SELECT_WRITE;
			if (FD_ISSET(handle, &eset)) retval |= SELECT_ERROR;
		} else if (sel != 0) {
			retval |= SELECT_ERROR;
		}

		retval &= mask;

		return retval;
	}

	std::ostream& operator<<(std::ostream& os, const ipv4_addr& addr);
	std::ostream& operator<<(std::ostream& os, const ipv4_socket_addr& saddr);
#endif
