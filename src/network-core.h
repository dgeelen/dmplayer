#ifndef NETWORK_CORE_H
	#define NETWORK_CORE_H
	#include "types.h"
	#include "network-crossplatform.h"
	
	struct ipv4_addr {
		union {
			uint32 full;
			struct { uint8 a,b,c,d; }component;
			uint8  array[4];
		};

		ipv4_addr() {
			full=0;
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
	
	class tcp_socket {
		public:
			tcp_socket( const ipv4_addr addr, const uint16 port );
			tcp_socket( const ipv6_addr addr, const uint16 port );
			void connect( const ipv4_addr addr, const uint16 port );
			void connect( const ipv6_addr addr, const uint16 port );
			bool bind( const ipv4_addr addr, const uint16 port );
			bool bind( const ipv6_addr addr, const uint16 port );
			
			uint32 send( const uint8* buf, const uint32 len );
			uint32 receive( const uint8* buf, const uint32 len );
			void disconnect();
		private:
			SOCKET sock;
	};

	class tcp_listen_socket {
		public:
			tcp_listen_socket();
			tcp_listen_socket(const ipv4_addr addr, const uint16 portnumber);
			tcp_listen_socket(const ipv6_addr addr, const uint16 portnumber);
			tcp_socket* accept();
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
			uint32 receive( ipv4_addr* from_addr, uint16* from_port, const uint8* buf, const uint32 len );
			uint32 receive( ipv6_addr* from_addr, uint16* from_port, const uint8* buf, const uint32 len );
		private:
			SOCKET sock;
	};
#endif