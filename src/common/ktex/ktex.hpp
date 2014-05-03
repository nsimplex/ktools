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


#ifndef KTOOLS_KTEX_HPP
#define KTOOLS_KTEX_HPP

#include "ktools_common.hpp"
#include "ktex/specs.hpp"
#include "binary_io_utils.hpp"

#include <squish/squish.h>

namespace KTools {
	namespace KTEX {
		class File : public NonCopyable {
		public:
			class Header : public HeaderSpecs {
				friend class File;
			public:
				typedef uint32_t data_t;
				data_t data;

				KTools::BinIOHelper io;
	
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
	
				void print(std::ostream& out, int verbosity = -1, size_t indentation = 0, const std::string& indent_string = "\t") const;
				std::ostream& dump(std::ostream& out) const;
				std::istream& load(std::istream& in);

				void reset() {
					data = 0;
					for(field_spec_iterator it = FieldSpecs.begin(); it != FieldSpecs.end(); ++it) {
						setField(it->first, it->second.value_default);
					}
				}

				Header() { reset(); }

				Header& operator=(const Header& h) {
					data = h.data;
					if(io.isUnknownSource()) {
						io.copySource(h.io);
					}
					else if(io.hasInverseSourceOf(h.io)) {
						io.reorder(data);
					}
					io.copyTarget(h.io);
					return *this;
				}
			};

			class Mipmap : public NonCopyable {
			public:
				typedef squish::u8 byte_t;

				const File* parent;

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

				Mipmap() : parent(NULL), data(NULL), datasz(0), width(0), height(0), pitch(0) {}

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

			struct CompressionFormat {
				bool is_uncompressed;
				int squish_flags;
			};

		public:
			Header header;
			KTools::BinIOHelper& io;
			
		private:
			Mipmap* Mipmaps;

			void deallocateMipmaps() {
				delete[] Mipmaps;
				Mipmaps = NULL;
			}

			void reallocateMipmaps(size_t howmany) {
				deallocateMipmaps();

				header.setField("mipmap_count", howmany);
				
				if(howmany > 0) {
					Mipmaps = new Mipmap[howmany];
					for(size_t i = 0; i < howmany; i++) {
						Mipmaps[i].parent = this;
					}
				}
			}

			CompressionFormat getCompressionFormat() const;

			// We use int for compliance with squish.
			Magick::Blob getRGBA(int& width, int& height) const;

			Magick::Image DecompressMipmap(const Mipmap& M, const CompressionFormat& fmt, int verbosity = -1) const;

			void CompressMipmap(Mipmap& M, const CompressionFormat& fmt, Magick::Image img, int verbosity = -1) const;

			bool flip_image;

		public:
			static bool isKTEXFile(std::istream& in);

			static bool isKTEXFile(const std::string& path);

			void flipImage(bool b) {
				flip_image = b;
			}

			void print(std::ostream& out, int verbosity = -1, size_t indentation = 0, const std::string& indent_string = "\t") const;
			std::ostream& dump(std::ostream& out, int verbosity = -1) const;
			std::istream& load(std::istream& in, int verbosity = -1, bool info_only = false);

			void dumpTo(const std::string& path, int verbosity = 1);
			void loadFrom(const std::string& path, int verbosity = -1, bool info_only = false);

			Magick::Image Decompress(int verbosity = -1) const {
				if(header.getField("mipmap_count") == 0) {
					return Magick::Image();
				}
				return DecompressMipmap(Mipmaps[0], getCompressionFormat(), verbosity);
			}

			template<typename OutputIterator>
			void Decompress(OutputIterator it, int verbosity = -1) const {
				const size_t num_mipmaps = header.getField("mipmap_count");
				CompressionFormat fmt = getCompressionFormat();
				for(size_t i = 0; i < num_mipmaps; i++) {
					*it++ = DecompressMipmap(Mipmaps[i], fmt, verbosity);
				}
			}

			void CompressFrom(Magick::Image img, int verbosity = -1) {
				std::list<Magick::Image> imglist;
				imglist.push_back(img);
				CompressFrom( imglist.begin(), imglist.end(), verbosity );
			}

			template<typename InputIterator>
			void CompressFrom(InputIterator first, InputIterator last, int verbosity = -1) {
				typedef typename InputIterator::value_type img_t;
				if(first == last) return;

				reallocateMipmaps( size_t(std::distance(first, last)) );

				Mipmap* M = Mipmaps;
				CompressionFormat fmt = getCompressionFormat();
				img_t img = *first++;

				if(verbosity >= 0) {
					std::cout << "Compressing " << img.columns() << "x" << img.rows() << " image into KTEX..." << std::endl;
				}

				while(true) {
					CompressMipmap( *M, fmt, img, verbosity );

					if(first == last)
						break;

					img = *first++;
					M++;
				}

				if(verbosity >= 0) {
					std::cout << "Compressed." << std::endl;
				}
			}

			File() : header(), io(header.io), Mipmaps(NULL), flip_image(true) {}
			virtual ~File() { deallocateMipmaps(); }
		};


	}
}

#endif
