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


#ifndef KTOOLS_IMAGE_OPERATIONS_HPP
#define KTOOLS_IMAGE_OPERATIONS_HPP

#include "ktools_common.hpp"
#include <functional>

namespace KTools { namespace ImOp {
	typedef std::unary_function<Magick::Image&, void> unary_operation_t;
	typedef std::unary_function<Magick::PixelPacket*, void> pixel_operation_t;

	class read : public unary_operation_t {
		const std::string& path;
	public:
		read(const std::string& p) : path(p) {}

		void operator()(Magick::Image& img) const {
			img.read(path);
		}
	};

	class write : public unary_operation_t {
		const std::string& path;
	public:
		write(const std::string& p) : path(p) {}

		void operator()(Magick::Image& img) const {
			img.write(path);
		}
	};

	template<typename Container>
	class SequenceWriter : public std::unary_function<const std::string&, void> {
		Container& c;
	public:
		SequenceWriter(Container& _c) : c(_c) {}

		void operator()(const std::string& pathSpec) {
			Magick::writeImages( c.begin(), c.end(), pathSpec, false );
		}
	};

	template<typename Container>
	inline SequenceWriter<Container> writeSequence(Container& c) {
		return SequenceWriter<Container>(c);
	}

	inline Magick::Quantum multiplyQuantum(Magick::Quantum q, double factor) {
		using namespace Magick;

		const double result = factor*q;
		if(result >= MaxRGB) {
			return MaxRGB;
		}
		else {
			return Quantum(result);
		}
	}

	class premultiplyPixelAlpha : public pixel_operation_t {
	public:
		void operator()(Magick::PixelPacket* p) const {
			using namespace Magick;

			double a = 1 - double(p->opacity)/MaxRGB;
			if(a <= 0.1) a = 0;
			else if(a >= 1) a = 1;

			p->red = multiplyQuantum(p->red, a);
			p->green = multiplyQuantum(p->green, a);
			p->blue = multiplyQuantum(p->blue, a);
		}
	};

	class demultiplyPixelAlpha : public pixel_operation_t {
	public:
		void operator()(Magick::PixelPacket* p) const {
			using namespace Magick;

			const double a = 1 - double(p->opacity)/MaxRGB;
			if(a <= 0 || a >= 1) return;

			const double inva = 1/a;

			p->red = multiplyQuantum(p->red, inva);
			p->green = multiplyQuantum(p->green, inva);
			p->blue = multiplyQuantum(p->blue, inva);
		}
	};

	template<typename PixelOperation>
	class pixelMap : public unary_operation_t {
		PixelOperation op;
	public:
		void operator()(Magick::Image& img) const {
			using namespace Magick;
			img.type(TrueColorMatteType);
			img.modifyImage();
			
			Pixels view(img);

			const size_t w = img.columns(), h = img.rows();
			{
				PixelPacket * RESTRICT p = view.get(0, 0, w, h);

				for(size_t i = 0; i < h; i++) {
					for(size_t j = 0; j < w; j++) {
						op(p++);
					}
				}
			}

			view.sync();
		}
	};

	/*
	void highlevel_premultiply(Magick::Image& img) const {
		Magick::Image old = img;
		img = Magick::Image(old.size(), "black");
		img.composite(old, 0, 0, Magick::OverCompositeOp);
		img.composite(old, 0, 0, Magick::CopyOpacityCompositeOp);
	}
	*/

	class premultiplyAlpha : public pixelMap<premultiplyPixelAlpha> {};

	class demultiplyAlpha : public pixelMap<demultiplyPixelAlpha> {};

	class cleanNoise : public unary_operation_t {
	public:
		cleanNoise() {}

		void operator()(Magick::Image& img) const {
			using namespace Magick;

			img.despeckle();

			Image alpha = img;
			alpha.channel(MatteChannel);
			alpha.negate();

			alpha.reduceNoise(1.6);

			img.matte(false);
			img.composite(alpha, 0, 0, CopyOpacityCompositeOp);

			img.matte(true);
		}
	};

	template<typename OpType>
	class SafeOperationWrapper : public std::unary_function<typename OpType::argument_type, typename OpType::result_type> {
		OpType op;

		typedef typename OpType::argument_type argument_type;
		typedef typename OpType::result_type result_type;

	public:
		SafeOperationWrapper(OpType _op) : op(_op) {}

		result_type operator()(argument_type x) {
			using namespace std;

			try {
				return op(x);
			}
			catch(Magick::WarningCoder& warning) {
				cerr << "Coder warning: " << warning.what() << endl;
			}
			catch(Magick::Warning& warning) {
				cerr << "Warning: " << warning.what() << endl;
			}
			catch(Magick::Error& err) {
				cerr << "Error: " << err.what() << endl;
				exit(MagickErrorCode);
			}
			catch(std::exception& err) {
				cerr << "Error: " << err.what() << endl;
				exit(GeneralErrorCode);
			}

			return result_type();
		}
	};

	template<typename OpType>
	inline SafeOperationWrapper<OpType> safeWrap(OpType op) {
		return SafeOperationWrapper<OpType>(op);
	}
}}

#endif
