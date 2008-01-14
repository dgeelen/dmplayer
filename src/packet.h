#ifndef PACKET_H
#define PACKET_H

#include "types.h"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <string>

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
				data[curpos++] = (var >> (i<<3)) & 0xFF;
		}

		template <typename T>
		typename boost::enable_if<boost::is_unsigned<T> >::type deserialize(T& var) {
			var = 0;
			for (int i = 0; i < sizeof(T); ++i)
				var |= data[curpos++] << (i<<3);
		}

		template <typename T> typename boost::enable_if<boost::is_unsigned<T>, T>::type deserialize() {
			T var = 0;
			for (int i = 0; i < sizeof(T); ++i)
				var |= data[curpos++] << (i<<3);
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
			for (uint i = 0; i < size; ++i)
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

#endif
