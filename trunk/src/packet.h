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
		packet() {
			curpos = 0;
		}
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

		template <typename T>
		typename boost::enable_if<boost::is_same<T, std::string> >::type
		serialize(const T& var) {
			uint32 size = var.size();
			serialize(size);
			for (uint i = 0; i < size; ++i)
				serialize<uint8>(var[i]);
		}

		template <typename T>
		typename boost::enable_if<boost::is_same<T, std::string> >::type
		deserialize(T& var) {
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

//NOTE: This is actually part of class packet, but needs to declared *outside* of class packet
//      for various obscure reasons.
//      See http://www.lrde.epita.fr/cgi-bin/twiki/view/Know/MemberFunctionsTemplateSpecialization
//          http://www.linuxquestions.org/questions/programming-9/explicit-specialization-in-non-namespace-scope-186352/
//          http://gcc.gnu.org/ml/gcc/1998-09/msg01216.html

#endif
