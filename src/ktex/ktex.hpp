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


#ifndef KTECH_KTEX_HPP
#define KTECH_KTEX_HPP

#include "ktech_common.hpp"
#include "ktex/specs.hpp"

namespace KTech {
	namespace KTEX {
		class File : public NonCopyable {
		public:
			class Header : public HeaderSpecs {
				friend class File;
			public:
				typedef uint32_t data_t;
				data_t data;
	
				// Assumes a little endian host layout.
				HeaderFieldSpec::value_t getField(const std::string& id) const {
					const HeaderFieldSpec& spec = FieldSpecs[id];

					if(!spec.isValid()) return HeaderFieldSpec::value_t();
	
					return (data >> spec.offset) & ((1 << spec.length) - 1);
				}

				const std::string& getFieldString(const std::string& id) const {
					static const std::string EmptyString;

					const HeaderFieldSpec& spec = FieldSpecs[id];

					if(!spec.isValid()) return EmptyString;

					return spec.normalize_value_inverse(getField(id));
				}
	
				template<typename T>
				void setField(const std::string& id, T val) {
					const HeaderFieldSpec& spec = FieldSpecs[id];
	
					data_t mask = ((1 << spec.length) - 1) << spec.offset;
	
					data_t maskedval = (spec.normalize_value(val) << spec.offset) & mask;
	
					data = (data & ~mask) | maskedval;
				}
				
				operator uint32_t() {
					return data;
				}
	
				void print(std::ostream& out, size_t indentation = 0, const std::string& indent_string = "\t") const;
				std::ostream& dump(std::ostream& out) const;
				std::istream& load(std::istream& in);

				void reset() {
					data = 0;
					for(field_spec_iterator it = FieldSpecs.begin(); it != FieldSpecs.end(); ++it) {
						setField(it->first, it->second.value_default);
					}
				}

				Header() { reset(); }
			};

			class Mipmap : public NonCopyable {
			public:
				typedef squish::u8 byte_t;

			private:
				friend class File;
				byte_t* data;
				uint32_t datasz;

			public:
				uint16_t width;
				uint16_t height;
				uint16_t pitch;

				const byte_t * getData() const {
					return data;
				}

				uint32_t getDataSize() const {
					return datasz;
				}

				void setDataSize(uint32_t sz) {
					delete[] data;
					if(sz != 0) {
						data = new byte_t[sz];
					} else {
						data = NULL;
					}
					datasz = sz;
				}

				Mipmap() : data(NULL), datasz(0), width(0), height(0), pitch(0) {}

				~Mipmap() {
					setDataSize(0);
				}

				/*
				 * The "pre" versions refer to the dumping/loading of metadata (width, etc.).
				 * The "post" versions refer to the dumping/loading of the raw data content.
				 */
				void print(std::ostream& out, size_t indentation = 0, const std::string& indent_string = "\t") const;
				std::ostream& dumpPre(std::ostream& out) const;
				std::ostream& dumpPost(std::ostream& out) const;
				std::istream& loadPre(std::istream& in);
				std::istream& loadPost(std::istream& in);
			};

		public:
			Header header;
			
		private:
			Mipmap* Mipmaps;

			void deallocateMipmaps() {
				delete[] Mipmaps;
				Mipmaps = NULL;
			}

			void reallocateMipmaps(size_t howmany) {
				deallocateMipmaps();
				
				if(howmany > 0)
					Mipmaps = new Mipmap[howmany];
			}

			// We use int for compliance with squish.
			Magick::Blob getRGBA(int& width, int& height) const;

		public:
			static bool isKTEXFile(const std::string& path);

			void print(std::ostream& out, size_t indentation = 0, const std::string& indent_string = "\t") const;
			std::ostream& dump(std::ostream& out) const;
			std::istream& load(std::istream& in);

			void dumpTo(const std::string& path);
			void loadFrom(const std::string& path);

			Magick::Image Decompress() const;
			void CompressFrom(Magick::Image img);

			File() : Mipmaps(NULL) {}
			~File() { deallocateMipmaps(); }
		};


	}
}

#endif
