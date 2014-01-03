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


#ifndef KTECH_IMAGE_OPERATIONS_HPP
#define KTECH_IMAGE_OPERATIONS_HPP

#include "ktech_common.hpp"
#include <functional>

namespace KTech { namespace ImOp {
	typedef std::unary_function<Magick::Image&, void> unary_operation_t;

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

	class premultiplyAlpha : public unary_operation_t {
	public:
		void operator()(Magick::Image& img) const {
			Magick::Image old = img;
			img = Magick::Image(old.size(), "black");
			img.composite(old, 0, 0, Magick::OverCompositeOp);
			img.composite(old, 0, 0, Magick::CopyOpacityCompositeOp);
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
