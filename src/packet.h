#ifndef PACKET_H
#define PACKET_H

#include <boost/serialization/base_object.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <map>
#include "types.h"
#include "playlist_management.h"

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

#define NETWERK_PROTOCOL_VERSION 2

class message {
	// is not used, but marks inheritance as virtual
	// this affect serialisation somehow, removing it breaks it all
	// why? how? what? where?
	private:
		virtual message* clone() { return new message(type); assert(false); };
	public:
		uint32 get_type() const { return type; };

		enum message_types {
			MSG_CONNECT=0,
			MSG_ACCEPT,
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
typedef boost::shared_ptr<message> messageref;
typedef boost::shared_ptr<const message> messagecref;


class message_connect : public message {
	public:
		message_connect() : message(MSG_CONNECT), version(NETWERK_PROTOCOL_VERSION) {};
		uint32 get_version() { return version; };
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
typedef boost::shared_ptr<message_connect> message_connect_ref;

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
typedef boost::shared_ptr<message_disconnect> message_disconnect_ref;

class message_accept : public message {
	public:
		message_accept() : message(MSG_ACCEPT) {};
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
		}
};
typedef boost::shared_ptr<message_accept> message_accept_ref;

class message_playlist_update : public message {
	public:
		enum update_type {
			UPDATE_CLEAR,
			UPDATE_MOVE,
			UPDATE_INSERT,
		};
	public:
		message_playlist_update() : message(MSG_PLAYLIST_UPDATE) {
			utype = UPDATE_CLEAR;
			data_list.clear();
		};
		message_playlist_update(const IPlaylist& playlist) : message(MSG_PLAYLIST_UPDATE) {
			utype = UPDATE_CLEAR;
			data_list.clear();
			BOOST_FOREACH(const Track& track, playlist)
				data_list.push_back(track);
		};
		message_playlist_update(Track track, uint32 pos = 0xFFFFFFFF) : message(MSG_PLAYLIST_UPDATE) {
			utype = UPDATE_INSERT;
			data_track = track;
			data_index = pos;

		};
		message_playlist_update(uint32 from, uint32 to = 0xFFFFFFFF) : message(MSG_PLAYLIST_UPDATE) {
			utype = UPDATE_MOVE;
			data_index = from;
			data_index2 = to;
		};

		void apply(IPlaylist* playlist) {
			switch (utype) {
				case UPDATE_CLEAR: {
					playlist->clear();
					BOOST_FOREACH(const Track& t, data_list)
						playlist->add(t);
				}; break;
				case UPDATE_INSERT: {
					if (data_index == 0xFFFFFFFF)
						playlist->add(data_track);
					else
						playlist->insert(data_index, data_track);
				}; break;
				case UPDATE_MOVE: {
					if (data_index2 == 0xFFFFFFFF)
						playlist->remove(data_index);
					else
						playlist->move(data_index, data_index2);
				}; break;
			};
		}
	private:
		update_type utype;
		std::vector<Track> data_list;
		Track data_track;
		uint32 data_index, data_index2;

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & utype;
			switch (utype) {
				case UPDATE_CLEAR: {
					ar & data_list;
				}; break;
				case UPDATE_INSERT: {
					ar & data_index;
					ar & data_track;
				}; break;
				case UPDATE_MOVE: {
					ar & data_index;
					ar & data_index2;
				}; break;
			};
		}
//		message_playlist_update() : message(MSG_PLAYLIST_UPDATE) {};
};
typedef boost::shared_ptr<message_playlist_update> message_playlist_update_ref;

#endif //PACKET_H
