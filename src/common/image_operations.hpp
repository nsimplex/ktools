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
#include "compat.hpp"
#include <functional>

namespace KTools {
	namespace KTEX {
		class File;
	}
namespace ImOp {
	typedef std::unary_function<Magick::Image&, void> basic_image_operation_t;
	typedef std::unary_function<Magick::PixelPacket*, void> basic_pixel_operation_t;
	typedef std::unary_function<KTEX::File&, void> basic_ktex_operation_t;

	typedef basic_image_operation_t basic_unary_operation_t;

	template<typename argument_type, typename result_type = void>
	class operation_t : public std::unary_function<argument_type, result_type> {
	public:
		typedef std::unary_function<argument_type, result_type> basic_operation_t;

		typedef operation_t type;

		/*
		operation_t() {}
		operation_t(const operation_t&) {}
		*/

		virtual ~operation_t() {}

		virtual result_type call(argument_type arg) const = 0;

		template<template<typename T, typename A> class Container, typename Alloc>
		void call(Container<argument_type, Alloc>& C) const {
			typedef typename Container<argument_type, Alloc>::iterator iterator;

			for(iterator it = C.begin(); it != C.end(); ++it) {
				call(*it);
			}
		}

		inline result_type operator()(argument_type arg) const {
			return call(arg);
		}

		template<typename T>
		inline result_type operator()(T& arg) const {
			return call(arg);
		}
	};

	template<typename first_argument_type, typename second_argument_type>
	class binary_operation_t : public std::binary_function<first_argument_type, second_argument_type, void> {
	public:
		typedef std::binary_function<first_argument_type, second_argument_type, void> basic_operation_t;

		typedef binary_operation_t type;

		virtual ~binary_operation_t() {}

		virtual void call(first_argument_type arg1, second_argument_type arg2) const = 0;

		inline void operator()(first_argument_type arg1, second_argument_type arg2) const {
			call(arg1, arg2);
		}
	};

	///

	typedef operation_t<Magick::Image&> image_operation_t;
	typedef image_operation_t unary_operation_t;

	typedef operation_t<KTEX::File&> ktex_operation_t;

	typedef operation_t<Magick::PixelPacket*> pixel_operation_t;

	///

	template<class Op>
	class reference_caller {};

	template<typename argument_type, typename result_type>
	class reference_caller< operation_t<argument_type, result_type> > : public operation_t<argument_type, result_type> {
	public:
		typedef operation_t<argument_type, result_type> basic_operation;

		virtual const basic_operation* get_pointer() const = 0;

		result_type docall(argument_type arg) const {
			return get_pointer()->call(arg);
		}

		virtual result_type call(argument_type arg) const {
			return docall(arg);
		}
	};

	template<typename Arg1, typename Arg2>
	class reference_caller< binary_operation_t<Arg1, Arg2> > : public binary_operation_t<Arg1, Arg2> {
	public:
		typedef binary_operation_t<Arg1, Arg2> basic_operation;

		virtual const basic_operation* get_pointer() const = 0;

		void docall(Arg1 arg1, Arg2 arg2) const {
			get_pointer()->call(arg1, arg2);
		}

		virtual void call(Arg1 arg1, Arg2 arg2) const {
			docall(arg1, arg2);
		}
	};

	///

	template<typename basic_operation>
	class operation_ref_t : public reference_caller< typename basic_operation::type > {
		Magick::Blob ref;
		
		void take_ownership(basic_operation* op, size_t sz) {
			ref.updateNoCopy(op, sz);
		}

		void copy_data(const basic_operation* op, size_t sz) {
			ref.update(op, sz);
		}

	public:
		operation_ref_t() { assert(ref.length() == 0); }

		template<typename derivative_type>
		explicit operation_ref_t(derivative_type* op) {
			take_ownership(op, sizeof(*op));
		}

		template<typename derivative_type>
		static inline operation_ref_t copy(const derivative_type& f) {
			return operation_ref_t(f);
		}

		operation_ref_t(const operation_ref_t& x) : ref(x.ref) {}

		template<typename derivative_type>
		operation_ref_t(const derivative_type& f) {
			take_ownership(new derivative_type(f), sizeof(f));
		}

		operation_ref_t& operator=(const operation_ref_t& x) {
			ref = x.ref;
			return *this;
		}

		virtual const basic_operation* get_pointer() const {
			if(empty()) {
				throw Error("Attempt to access null reference.");
			}
			return reinterpret_cast<const basic_operation*>(ref.data());
		}

		inline const basic_operation& deference() const {
			return *get_pointer();
		}

		const basic_operation& operator*() const {
			return deference();
		}

		const basic_operation* operator->() const {
			return get_pointer();
		}

		operator const basic_operation&() const {
			return deference();
		}

		inline bool empty() const {
			return ref.length() == 0;
		}

		inline operator bool() const {
			return !empty();
		}

		inline bool operator==(Nil) const {
			return empty();
		}
	};

	///

	template<typename argument_type>
	class operation_chain : public operation_t<argument_type> {
	public:
		typedef operation_t<argument_type> element_type;
		typedef operation_ref_t<element_type> reference_type;

	private:
		typedef std::deque<reference_type> chain_t;
		typedef typename chain_t::iterator iterator;
		typedef typename chain_t::const_iterator const_iterator;

		chain_t chain;

	public:
		operation_chain() : chain() {}

		void clear() {
			chain.clear();
		}

		virtual ~operation_chain() {
			clear();
		}

		virtual void call(argument_type arg) const {
			for(const_iterator it = chain.begin(); it != chain.end(); ++it) {
				it->call(arg);
			}
		}

		template<typename derivative_type>
		void push_back(const derivative_type* f) {
			chain.push_back( reference_type(f) );
		}

		template<typename derivative_type>
		void push_back(const derivative_type& f) {
			chain.push_back( reference_type::copy(f) );
		}

		operation_chain& operator<<(const element_type* f) {
			push_back(f);
			return *this;
		}
	};

	typedef operation_chain<Magick::Image&> image_operation_chain;
	typedef image_operation_chain unary_operation_chain;

	typedef operation_chain<KTEX::File&> ktex_operation_chain;

	///

	class read : public unary_operation_t {
		std::string path;
	public:
		read(const std::string& p) : path(p) {}
		read(const read& r) {*this = r;}

		read& operator=(const read& r) {
			path = r.path;
			return *this;
		}

		virtual void call(Magick::Image& img) const {
			img.read(path);
		}
	};

	class write : public unary_operation_t {
		Compat::Path path;
	public:
		static void prepare(const Compat::Path& p, Magick::Image& img) {
			if(p.hasExtension("png")) {
				img.magick("png");
				img.defineValue("png", "color-type", "6");
			}
		}

		write(const Compat::Path& p) : path(p) {}
		write(const write& w) {*this = w;}

		write& operator=(const write& w) {
			path = w.path;
			return *this;
		}

		virtual void call(Magick::Image& img) const {
			prepare(path, img);
			img.write(path);
		}
	};

	template<typename Container>
	class SequenceWriter : public operation_t<const Compat::Path&> {
		Container* c;
	public:
		typedef typename Container::iterator img_iterator;

		SequenceWriter(Container& _c) : c(&_c) {}
		SequenceWriter(Container* _c) : c(_c) {}

		SequenceWriter& operator=(const SequenceWriter& sw) {
			c = sw.c;
			return *this;
		}

		virtual void call(const Compat::Path& pathSpec) const {
			for(img_iterator it = c->begin(); it != c->end(); ++it) {
				write::prepare(pathSpec, *it);
			}
			Magick::writeImages( c->begin(), c->end(), pathSpec, false );
		}
	};

	template<typename Container>
	inline SequenceWriter<Container> writeSequence(Container& c) {
		return SequenceWriter<Container>(c);
	}

	inline Magick::Quantum multiplyQuantum(Magick::Quantum q, double factor) {
		using namespace Magick;

		const double result = factor*q;
		if(result >= QuantumRange) {
			return QuantumRange;
		}
		else {
			return Quantum(result);
		}
	}

	class premultiplyPixelAlpha : public pixel_operation_t {
	public:
		virtual void call(Magick::PixelPacket* p) const {
			using namespace Magick;

			double a = 1 - double(p->opacity)/QuantumRange;
			if(a <= 0.1) a = 0;
			else if(a >= 1) a = 1;

			p->red = multiplyQuantum(p->red, a);
			p->green = multiplyQuantum(p->green, a);
			p->blue = multiplyQuantum(p->blue, a);
		}
	};

	class demultiplyPixelAlpha : public pixel_operation_t {
	public:
		virtual void call(Magick::PixelPacket* p) const {
			using namespace Magick;

			const double a = 1 - double(p->opacity)/QuantumRange;
			if(a <= 0 || a >= 1) return;

			const double inva = 1/a;

			p->red = multiplyQuantum(p->red, inva);
			p->green = multiplyQuantum(p->green, inva);
			p->blue = multiplyQuantum(p->blue, inva);
		}
	};

	template<typename PixelOperation>
	class pixelMap : public image_operation_t {
		PixelOperation op;
	public:
		virtual void call(Magick::Image& img) const {
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

	class cleanNoise : public image_operation_t {
	public:
		cleanNoise() {}

		virtual void call(Magick::Image& img) const {
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
}}

#endif
