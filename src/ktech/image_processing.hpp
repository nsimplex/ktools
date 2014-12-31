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


#include "ktech.hpp"
#include "image_operations.hpp"
#include "file_abstraction.hpp"
#include "ktex/ktex.hpp"

namespace KTech {
	static inline bool should_resize() {
		return options::width != nil || options::height != nil || options::pow2 || options::force_square;
	}
}

namespace KTools {
namespace ImOp {
	using namespace KTech;

	class ktexHeaderSetter : public ktex_operation_t {
		KTEX::File::Header h;

	public:
		ktexHeaderSetter(KTEX::File::Header _h) : h(_h) {}
		ktexHeaderSetter(const ktexHeaderSetter& hs) : h(hs.h) {}

		virtual void call(KTEX::File& ktex) const {
			ktex.header = h;
		}
	};

	class imageResizer : public image_operation_t {
	public:
		virtual void call(Magick::Image& img) const {
			Magick::Geometry size = img.size();
			const size_t w0 = size.width(), h0 = size.height();

			size.aspect(true);

			if(options::width != nil && options::height != nil) {
				size.width(options::width);
				size.height(options::height);
			}
			else if(options::width != nil) {
				size.width(options::width);
				size.height((h0*size.width())/w0);
			}
			else if(options::height != nil) {
				size.height(options::height);
				size.width((w0*size.height())/h0);
			}

			if(options::pow2) {
				size.width(BitOp::Pow2Rounder::roundUp(size.width()));
				size.height(BitOp::Pow2Rounder::roundUp(size.height()));
			}

			if(options::force_square) {
				if(size.width() < size.height()) {
					size.width(size.height());
				}
				else if(size.height() < size.width()) {
					size.height(size.width());
				}
			}

			std::string operation_verb;
			if(options::extend) {
				if(options::extend_left) {
					if(size.width() > w0) {
						size.xOff( size.width() - w0 );
					}
				}
				img.backgroundColor("transparent");
				img.extent( size );
				if(options::verbosity >= 0) {
					operation_verb = "Extended";
				}
			}
			else {
				img.filterType( options::filter );
				img.resize( size );
				if(options::verbosity >= 0) {
					operation_verb = "Resized";
				}
			}

			if(options::verbosity >= 0) {
				std::cout << operation_verb << " image to " << img.columns() << "x" << img.rows() << std::endl;
			}
		}
	};


	template<typename ImageContainer>
	static inline void generate_mipmaps(ImageContainer& imgs) {
		typedef typename ImageContainer::value_type img_t;

		img_t img = imgs.front();

		size_t width = img.columns();
		size_t height = img.rows();

		if(options::verbosity >= 1) {
			const size_t mipmap_count = BitOp::countBinaryDigits( std::min(width, height) );
			std::cout << "Generating " << mipmap_count << " mipmaps..." << std::endl;
		}

		width /= 2;
		height /= 2;

		while(width > 0 || height > 0) {
			img.filterType( options::filter );
			img.resize( Magick::Geometry(std::max(width, size_t(1)), std::max(height, size_t(1))) );
			//img.despeckle();
			imgs.push_back( img );

			width /= 2;
			height /= 2;
		}
	}

	class ktexCompressor : public binary_operation_t<KTEX::File&, Magick::Image> {
		ktexHeaderSetter setheader;
		const int verbosity;

	public:
		ktexCompressor(KTEX::File::Header h, int _v = -1) : setheader(h), verbosity(_v) {}

		template<typename image_container_t>
		void compress(KTEX::File& tex, image_container_t& imgs) const {
			typedef typename image_container_t::iterator image_iterator_t;

			if(should_resize()) {
				imageResizer()( imgs.front() );
			}

			if(options::no_mipmaps || imgs.size() > 1) {
				if(options::verbosity >= 1) {
					std::cout << "Skipping mipmap generation..." << std::endl;
				}
			} else {
				generate_mipmaps( imgs );
			}

			{
				image_iterator_t first_secondary_mipmap = imgs.begin();
				std::advance(first_secondary_mipmap, 1);
				std::for_each( first_secondary_mipmap, imgs.end(), ImOp::cleanNoise() );
			}

			if(!options::no_premultiply) {
				if(verbosity >= 1) {
					std::cout << "Premultiplying alpha..." << std::endl;
				}
				std::for_each( imgs.begin(), imgs.end(), ImOp::premultiplyAlpha() );
			}
			else if(verbosity >= 1) {
				std::cout << "Skipping alpha premultiplication..." << std::endl;
			}

			setheader(tex);
			tex.CompressFrom(imgs.begin(), imgs.end(), verbosity);
		}

		void compress(KTEX::File& tex, Magick::Image img) const {
			const int verbosity0 = options::verbosity;
			options::verbosity = std::min(verbosity0, verbosity);
			std::deque<Magick::Image> imgs;
			imgs.push_back(img);
			compress(tex, imgs);
			options::verbosity = verbosity0;
		}

		virtual void call(KTEX::File& tex, Magick::Image img) const {
			compress(tex, img);
		}
	};

	class ktexDecompressor : public operation_t<const KTEX::File&, Magick::Image> {
		const int verbosity;

		bool multiple_mipmaps;

		template<typename image_container_t>
		void do_decompress(const KTEX::File& tex, image_container_t& imgs, bool _mult_mipmaps) const {
			const int verbosity0 = options::verbosity;
			options::verbosity = std::min(verbosity0, verbosity);

			if(_mult_mipmaps) {
				tex.Decompress( std::back_inserter(imgs), verbosity );
			}
			else {
				imgs.clear();
				imgs.push_back( tex.Decompress(verbosity) );
			}

			if(!options::no_premultiply) {
				if(verbosity >= 1) {
					std::cout << "Demultiplying alpha..." << std::endl;
				}
				std::for_each( imgs.begin(), imgs.end(), ImOp::demultiplyAlpha() );
			}
			else if(verbosity >= 1) {
				std::cout << "Skipping alpha demultiplication..." << std::endl;
			}

			if(should_resize()) {
				if(imgs.size() > 1) {
					throw Error("Attempt to resize a mipchain.");
				}
				imageResizer()( imgs.front() );
			}

			std::for_each( imgs.begin(), imgs.end(), Magick::qualityImage(options::image_quality) );

			options::verbosity = verbosity0;
		}

	public:
		ktexDecompressor(int _v = -1, bool _mult_mipmaps = false) : verbosity(_v), multiple_mipmaps(_mult_mipmaps) {}

		template<typename image_container_t>
		void decompress(const KTEX::File& tex, image_container_t& imgs) const {
			do_decompress(tex, imgs, multiple_mipmaps);
		}

		Magick::Image decompress(const KTEX::File& tex) const {
			std::vector< Magick::Image > imgs;
			imgs.reserve(1);
			do_decompress(tex, imgs, false);
			return imgs.front();
		}

		virtual Magick::Image call(const KTEX::File& tex) const {
			return decompress(tex);
		}
	};
}}
