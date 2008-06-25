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
#include "network/network-core.h"


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
	private:
		uint8 databuf[PACKET_SIZE];
		int curpos;
	public:

		packet() {
			reset();
		}

		uint8* data() {
			return databuf;
		}

		uint32 size() {
			return data_length();
		}

		uint32 maxsize() {
			return PACKET_SIZE;
		}

		template <typename T>
		typename boost::enable_if<boost::is_unsigned<T> >::type serialize(const T& var) {
			for (int i = 0; i < sizeof(T); ++i)
				databuf[curpos++] = (uint8)((var >> (i<<3)) & 0xFF);
		}

		template <typename T>
		typename boost::enable_if<boost::is_unsigned<T> >::type
		deserialize(T& var) {
			var = 0;
			for (int i = 0; i < sizeof(T); ++i)
				var |= databuf[curpos++] << (i<<3);
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
			MSG_VOTE,
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
		message_accept(ClientID cid_) : message(MSG_ACCEPT), cid(cid_) {};

		ClientID cid;
	private:
		message_accept() {};

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & cid;
		}
};
typedef boost::shared_ptr<message_accept> message_accept_ref;

class message_vote : public message {
	public:
		message_vote(TrackID id_, bool is_min_vote_) : message(MSG_VOTE), id(id_), is_min_vote(is_min_vote_) {};
		bool is_min_vote;
		TrackID getID() { return id; };
	private:
		TrackID id;

		message_vote() {};

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & id;
			ar & is_min_vote;
		}
};
typedef boost::shared_ptr<message_vote> message_vote_ref;

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

class message_query_trackdb : public message {
	public:
		message_query_trackdb(uint32 qid_, Track search_) : qid(qid_), message(MSG_QUERY_TRACKDB), search(search_) {};

		uint32 qid;
		Track search;
	private:

		message_query_trackdb() {};

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & search;
			ar & qid;
		}
};
typedef boost::shared_ptr<message_query_trackdb> message_query_trackdb_ref;


class message_query_trackdb_result : public message {
	public:
		message_query_trackdb_result(uint32 qid_, std::vector<Track> result_) : qid(qid_), message(MSG_QUERY_TRACKDB_RESULT), result(result_) {};


		uint32 qid;
		std::vector<Track> result;
	private:

		message_query_trackdb_result() {};

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & result;
			ar & qid;
		}
};
typedef boost::shared_ptr<message_query_trackdb_result> message_query_trackdb_result_ref;

class message_request_file : public message {
	public:
		message_request_file(const TrackID& id_) : message(MSG_REQUEST_FILE), id(id_) {};
		TrackID id;
	private:
		message_request_file() {};

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & id;
		}
};
typedef boost::shared_ptr<message_request_file> message_request_file_ref;

class message_request_file_result : public message {
	public:
		message_request_file_result(const std::vector<uint8>& data_, TrackID id_) : message(MSG_REQUEST_FILE_RESULT), data(data_), id(id_) {};
		TrackID id;
		std::vector<uint8> data;
	private:
		message_request_file_result() {};

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version) {
			ar & boost::serialization::base_object<message>(*this);
			ar & data;
			ar & id;
		}
};
typedef boost::shared_ptr<message_request_file_result> message_request_file_result_ref;

void operator<<(tcp_socket& sock, const messagecref msg);
void operator>>(tcp_socket& sock,       messageref& msg);

#endif //PACKET_H
