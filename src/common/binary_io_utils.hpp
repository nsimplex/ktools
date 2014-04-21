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


#ifndef KTOOLS_IO_UTILS_HPP
#define KTOOLS_IO_UTILS_HPP

#include "ktools_common.hpp"

#include <boost/detail/endian.hpp>
#include <algorithm>

namespace KTools {
	class BinIOHelper {
	private:
		/*
		 * I account here only for little and big endian.
		 */
		enum Endianess {
			ktools_LITTLE_ENDIAN,
			ktools_BIG_ENDIAN,

			ktools_UNKNOWN_ENDIAN
		};

#if defined(BOOST_LITTLE_ENDIAN)
		static const Endianess ktools_NATIVE_ENDIANNESS = ktools_LITTLE_ENDIAN;
		static const Endianess ktools_INVERSE_NATIVE_ENDIANNESS = ktools_BIG_ENDIAN;
#elif defined(BOOST_BIG_ENDIAN)
		static const Endianess ktools_NATIVE_ENDIANNESS = ktools_BIG_ENDIAN;
		static const Endianess ktools_INVERSE_NATIVE_ENDIANNESS = ktools_LITTLE_ENDIAN;
#else
#	error Unsupported native endianness.
#endif

		Endianess source_endianness;
		Endianess target_endianness;

	public:
		BinIOHelper() : source_endianness(ktools_UNKNOWN_ENDIAN), target_endianness(ktools_LITTLE_ENDIAN) {}

		void setNativeSource() {
			source_endianness = ktools_NATIVE_ENDIANNESS;
		}
		void setInverseNativeSource() {
			source_endianness = ktools_INVERSE_NATIVE_ENDIANNESS;
		}
		void setLittleSource() {
			source_endianness = ktools_LITTLE_ENDIAN;
		}
		void setBigSource() {
			source_endianness = ktools_BIG_ENDIAN;
		}
		bool isNativeSource() const {
			return source_endianness == ktools_NATIVE_ENDIANNESS;
		}
		bool isInverseNativeSource() const {
			return source_endianness == ktools_INVERSE_NATIVE_ENDIANNESS;
		}
		bool isLittleSource() const {
			return source_endianness == ktools_LITTLE_ENDIAN;
		}
		bool isBigSource() const {
			return source_endianness == ktools_BIG_ENDIAN;
		}
		bool isUnknownSource() const {
			return source_endianness == ktools_UNKNOWN_ENDIAN;
		}

		void setNativeTarget() {
			target_endianness = ktools_NATIVE_ENDIANNESS;
		}
		void setInverseNativeTarget() {
			target_endianness = ktools_INVERSE_NATIVE_ENDIANNESS;
		}
		void setLittleTarget() {
			target_endianness = ktools_LITTLE_ENDIAN;
		}
		void setBigTarget() {
			target_endianness = ktools_BIG_ENDIAN;
		}
		bool isNativeTarget() const {
			return target_endianness == ktools_NATIVE_ENDIANNESS;
		}
		bool isInverseNativeTarget() const {
			return target_endianness == ktools_INVERSE_NATIVE_ENDIANNESS;
		}
		bool isLittleTarget() const {
			return target_endianness == ktools_LITTLE_ENDIAN;
		}
		bool isBigTarget() const {
			return target_endianness == ktools_BIG_ENDIAN;
		}

		bool hasInverseSourceOf(const BinIOHelper& h) const {
			if(source_endianness == ktools_LITTLE_ENDIAN) {
				return h.source_endianness == ktools_BIG_ENDIAN;
			}
			else if(source_endianness == ktools_BIG_ENDIAN) {
				return h.source_endianness == ktools_LITTLE_ENDIAN;
			}
			return false;
		}

		void copySource(const BinIOHelper& h) {
			source_endianness = h.source_endianness;
		}
		void copyTarget(const BinIOHelper& h) {
			target_endianness = h.target_endianness;
		}

		
		template<typename T>
		static void raw_read_integer(std::istream& in, T& n) {
			in.read( reinterpret_cast<char*>( &n ), sizeof( n ) );
		}

		template<typename T>
		static void raw_write_integer(std::ostream& out, T n) {
			out.write( reinterpret_cast<const char*>( &n ), sizeof( n ) );
		}

		// Based on boost's, to some extent.
		template<typename T>
		static void reorder(T& n) {
			char *nptr = reinterpret_cast<char*>(&n);
			std::reverse(nptr, nptr + sizeof(n));
		}

		template<typename T>
		inline void read_integer(std::istream& in, T& n) const {
			if(source_endianness == ktools_UNKNOWN_ENDIAN) {
				throw Error("Undefined source endianness.");
			}

			raw_read_integer(in, n);
			
			if(source_endianness == ktools_INVERSE_NATIVE_ENDIANNESS) {
				reorder(n);
			}
		}

		template<typename T>
		inline void write_integer(std::ostream& out, T n) const {
			if(target_endianness == ktools_UNKNOWN_ENDIAN) {
				throw Error("Undefined target endianness.");
			}

			if(target_endianness == ktools_INVERSE_NATIVE_ENDIANNESS) {
				reorder(n);
			}

			raw_write_integer(out, n);
		}

		template<typename T>
		inline void read_float(std::istream& in, T& x) const {
			read_integer(in, x);
		}

		template<typename T>
		inline void write_float(std::ostream& out, T x) const {
			write_integer(out, x);
		}
	};
}

#endif
