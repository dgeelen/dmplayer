#ifndef PACKET_H
#define PACKET_H

#include <boost/serialization/base_object.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <string>
#include <map>
#include "types.h"

/** ** ** ** ** ** ** ** **
 *       UDP Packet       *
 ** ** ** ** ** ** ** ** **/

enum packet_types {
	PT_QUERY_SERVERS=0,
	PT_REPLY_SERVERS,
};

//TODO: Needs to be configurable @runtime
#define PACKET_SIZE 1400

class packet {
	public:
		uint8 data[PACKET_SIZE];

		packet() {
			reset();
		}

		template <typename T>
		typename boost::enable_if<boost::is_unsigned<T> >::type serialize(const T& var) {
			for (int i = 0; i < sizeof(T); ++i)
				data[curpos++] = (uint8)((var >> (i<<3)) & 0xFF);
		}

		template <typename T>
		typename boost::enable_if<boost::is_unsigned<T> >::type
		deserialize(T& var) {
			var = 0;
			for (int i = 0; i < sizeof(T); ++i)
				var |= data[curpos++] << (i<<3);
		}

		template <typename T>
		T
		deserialize() {
			T var = 0;
			deserialize<T>(var);
			return var;
		}

		uint32 data_length() {
			return curpos;
		}

		void reset() {
			curpos = 0;
		}


		void serialize(const std::string& var) {
			uint32 size = var.size();
			serialize(size);
			for (uint i = 0; i < size ; ++i)
				serialize<uint8>(var[i]);
		}


		void deserialize(std::string& var) {
			uint32 size;
			deserialize(size);
			var.resize(size);
			for (uint i = 0; i < size; ++i) {
				uint8 t;
				deserialize(t);
				var[i] = t;
			}
		}
		private:
			int curpos;
};

/** ** ** ** ** ** ** ** **
 *      TCP Packets       *
 ** ** ** ** ** ** ** ** **/

/* Typical usage scenario:*
 *  .--------.       .----------.      .--------.
 *  | USER_A |       | 'SERVER' |      | USER_B |
 *  '--------'       '----------'      '--------'
 *      |                 |                |
 *      |     Connect     |                |
 *      |---------------->|                |       # User A wants to connect to the server (request to connect)
 *      |     Disconnect  |                |
 *      |<----------------|                |       # A's request is denied   (server closes TCP connection)
 *      |     Connect     |                |
 *      |---------------->|                |       # A has altered his settings and tries again
 *      |     Connect     |                |
 *      |<----------------|                |       # Now A's request is accepted (connect message from server->client)
 *      | PlaylistUpdate  |                |
 *      |<----------------|                |       # The server informs A of the current state of the playlist
 *      |  QueryTrackDB   |                |
 *      |---------------->|                |       # Next user A wants to search (for a song, by title, artist, etc)
 *      |  QueryTrackDB   |  QueryTrackDB  |
 *      |<----------------|--------------->|       # The server forwards A's request to all clients (1)
 *      |   QTDB_Result   |                |
 *      |---------------->|  QTDB_Result   |       # A searches his private TrackDB and returns the results (2)
 *      |   QTDB_Result   |<---------------|       # So does B
 *      |<----------------|                |       # The server aggregates and returns the results to A (3)(4)
 *      |      Vote       |                |
 *      |---------------->|                |       # A find a nice song in the results and adds it to the playlist
 *      | PlaylistUpdate  | PlaylistUpdate |
 *      |<----------------|--------------->|       # The playlist is updated and all clients are notified of the new situation
 *      |   RequestFile   |                |
 *      |-----------------+--------------->|       # When it's time to play the song, A requests the file from B (4)
 *      |                 | ReqFileResult  |
 *      |<----------------+----------------|       # B transfers the file to A
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *      |                 |                |
 *
 *  (1) Possibly A is omitted and performs the search locally
 *  (2) If (1), then A does not return the results to the server, saving a network round trip
 *  (3) After receiving the results from all other clients A must add his own results from (1)
 *  (4) The result of the QueryTrackDB is an std::vector<TrackID>, so that A knows where each file can be downloaded
 */


#define NETWERK_PROTOCOL_VERSION 0

class message {
	public:
		uint32 get_type() const { return type; };
		enum message_types {
			MSG_CONNECT=0,
			MSG_DISCONNECT,
			MSG_PLAYLIST_UPDATE,
			MSG_QUERY_TRACKDB,
			MSG_QUERY_TRACKDB_RESULT,
			MSG_REQUEST_FILE,
			MSG_REQUEST_FILE_RESULT,
		};
	message() : type(-1) {};
	protected:
		message(uint32 type_) : type(type_) {};
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & type;
		}

		uint32 type;
};


class message_connect : public message {
	public:
		message_connect() : message(MSG_CONNECT), version(NETWERK_PROTOCOL_VERSION) {};
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			// IMPORTANT! Don't *EVER* try to serialize the 'const unsigned int version' in the method definition!
			ar & this->version;
		}
		uint32 version;
};

class message_disconnect : public message {
	public:
		message_disconnect() : message(MSG_DISCONNECT) {};
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
		}
};

#endif //PACKET_H
