/*
Copyright (C) 2013  simplex

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef KTECH_KTEX_IO_HPP
#define KTECH_KTEX_IO_HPP

#include "ktech_common.hpp"

#include <boost/detail/endian.hpp>
#include <algorithm>

namespace KTech {
	/*
	 * I account here only for little and big endian.
	 */
	enum Endianess {
		ktech_LITTLE_ENDIAN,
		ktech_BIG_ENDIAN,

		ktech_UNKNOWN_ENDIAN
	};

#if defined(BOOST_LITTLE_ENDIAN)
	static const Endianess ktech_NATIVE_ENDIANNESS = ktech_LITTLE_ENDIAN;
	static const Endianess ktech_INVERSE_NATIVE_ENDIANNESS = ktech_BIG_ENDIAN;
#elif defined(BOOST_BIG_ENDIAN)
	static const Endianess ktech_NATIVE_ENDIANNESS = ktech_BIG_ENDIAN;
	static const Endianess ktech_INVERSE_NATIVE_ENDIANNESS = ktech_LITTLE_ENDIAN;
#else
#	error Unsupported native endianness.
#endif

	namespace KTEX {
		extern Endianess source_endianness;
		extern Endianess target_endianness;

		template<typename T>
		inline void raw_read_integer(std::istream& in, T& n) {
			in.read( reinterpret_cast<char*>( &n ), sizeof( n ) );
		}

		template<typename T>
		inline void raw_write_integer(std::ostream& out, T n) {
			out.write( reinterpret_cast<const char*>( &n ), sizeof( n ) );
		}

		// Based on boost's, to some extent.
		template<typename T>
		inline void reorder(T& n) {
			char *nptr = reinterpret_cast<char*>(&n);
			std::reverse(nptr, nptr + sizeof(n));
		}

		template<typename T>
		inline void read_integer(std::istream& in, T& n) {
			if(source_endianness == ktech_UNKNOWN_ENDIAN) {
				throw Error("Undefined endianness for reading.");
			}

			raw_read_integer(in, n);
			
			if(source_endianness == ktech_INVERSE_NATIVE_ENDIANNESS) {
				reorder(n);
			}
		}

		template<typename T>
		inline void write_integer(std::ostream& out, T n) {
			if(target_endianness == ktech_UNKNOWN_ENDIAN) {
				throw Error("Undefined endianness for writing.");
			}

			if(target_endianness == ktech_INVERSE_NATIVE_ENDIANNESS) {
				reorder(n);
			}

			raw_write_integer(out, n);
		}
	}
}

#endif
