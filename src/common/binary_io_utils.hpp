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
#include "metaprogramming.hpp"

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


		static inline void read_fail() {
			throw KToolsError("failed to read data from stream. It might be corrupt.");
		}

		static inline void write_fail() {
			throw KToolsError("failed to write data to stream.");
		}


		template<typename T>
		static void raw_read_value(std::istream& in, T& n) {
			if(!in.read( reinterpret_cast<char*>( &n ), sizeof( n ) )) {
				read_fail();
			}
		}

		template<typename T>
		static void raw_write_value(std::ostream& out, const T n) {
			if(!out.write( reinterpret_cast<const char*>( &n ), sizeof( n ) )) {
				write_fail();
			}
		}

		/*
		 * Reads a value affected by endianness.
		 */
		template<typename T>
		inline void read_ordered_value(std::istream& in, T& n) const {
			if(source_endianness == ktools_UNKNOWN_ENDIAN) {
				throw Error("Undefined source endianness.");
			}

			raw_read_value(in, n);
			
			if(source_endianness == ktools_INVERSE_NATIVE_ENDIANNESS) {
				reorder(n);
			}
		}

		/*
		 * Writes a value affected by endianness.
		 */
		template<typename T>
		inline void write_ordered_value(std::ostream& out, T n) const {
			if(target_endianness == ktools_UNKNOWN_ENDIAN) {
				throw Error("Undefined target endianness.");
			}

			if(target_endianness == ktools_INVERSE_NATIVE_ENDIANNESS) {
				reorder(n);
			}

			raw_write_value(out, n);
		}

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
			staticAssertIntegral<T>();
			raw_read_value(in, n);
		}

		template<typename T>
		static void raw_write_integer(std::ostream& out, const T n) {
			staticAssertIntegral<T>();
			raw_write_value(out, n);
		}

		// Based on boost's, to some extent.
		template<typename T>
		static void reorder(T& n) {
			char *nptr = reinterpret_cast<char*>(&n);
			std::reverse(nptr, nptr + sizeof(n));
		}

		template<typename T>
		inline void read_integer(std::istream& in, T& n) const {
			staticAssertIntegral<T>();
			read_ordered_value(in, n);
		}

		template<typename T>
		inline void write_integer(std::ostream& out, const T n) const {
			staticAssertIntegral<T>();
			write_ordered_value(out, n);
		}

		template<typename T>
		inline void read_float(std::istream& in, T& x) const {
			staticAssertFloating<T>();
			read_ordered_value(in, x);
		}

		template<typename T>
		inline void write_float(std::ostream& out, const T x) const {
			staticAssertFloating<T>();
			write_ordered_value(out, x);
		}

		static inline void read_string(std::istream& in, std::string& str, const size_t len) {
			char buffer[1 << 15];
			if(len > sizeof(buffer)) {
				throw KToolsError( strformat("Attempt to read a string larger than %lu bytes. The stream might be corrupt.", (unsigned long)sizeof(buffer)) );
			}

			if(!in.read(buffer, len)) {
				read_fail();
			}

			str.assign(buffer, len);
		}

		template<typename LengthType>
		inline void read_len_string(std::istream& in, std::string& str) const {
			LengthType len;
			read_integer(in, len);
			read_string(in, str, len);
		}

		template<typename LengthType>
		inline void skip_len_string(std::istream& in) const {
			LengthType len;
			read_integer(in, len);
			in.ignore(len);
		}

		/*
		static inline void write_string(std::ostream& out, const std::string& str) {
			if(!out.write(str.data(), str.length()) {
				write_fail();
			}
		}
		*/

		template<typename LengthType>
		inline void write_len_string(std::ostream& out, const std::string& str) const {
			const LengthType len = LengthType(str.length());
			write_integer(out, len);
			if(!out.write(str.data(), len)) {
				write_fail();
			}
		}

		template<typename charT, typename charTraits>
		static inline void sanitizeStream(std::basic_ios<charT, charTraits>& s) {
			s.imbue(std::locale::classic());
		}

		static inline void openBinaryStream(std::ifstream& in, const std::string& fname, bool validate = true) {
			in.open(fname.c_str(), std::ifstream::in | std::ifstream::binary);
			if(validate) {
				check_stream_validity(in, fname);
			}
			sanitizeStream(in);
		}

		static inline void openBinaryStream(std::ofstream& out, const std::string& fname, bool validate = true) {
			out.open(fname.c_str(), std::ifstream::out | std::ifstream::binary);
			if(validate) {
				check_stream_validity(out, fname);
			}
			sanitizeStream(out);
		}

		static inline uint32_t getMagicNumber(std::istream& in, bool rewind = true) {
			std::streampos p;
			
			if(rewind) {
				p = in.tellg();

				if(p < 0) {
					throw KToolsError("Input stream is not seekable.");
				}

				in.seekg(0, in.beg);
			}

			uint32_t magic;
			raw_read_integer(in, magic);

			if(rewind) {
				in.seekg(p, in.beg);
			}

			return magic;
		}

		static inline uint32_t getMagicNumber(const std::string& fname, bool rewind = true) {
			std::ifstream in;
			openBinaryStream(in, fname, true);
			return getMagicNumber(in, rewind);
		}
	};

	typedef BinIOHelper BinaryIOHelper;
}

#endif
