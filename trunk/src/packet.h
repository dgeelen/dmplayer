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
