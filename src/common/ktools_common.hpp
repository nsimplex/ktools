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


#ifndef KTOOLS_BASIC_HPP
#define KTOOLS_BASIC_HPP


#include <config.h>

#include "compat.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

extern "C" {
#if defined(HAVE_STDINT_H)
#	include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#	include <inttypes.h>
#elif defined(HAVE_SYS_TYPES_H)
#	include <sys/types.h>
#endif
}

#include <exception>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <iterator>
#include <algorithm>
#include <functional>

#include <Magick++.h>


#ifndef HAVE_SNPRINTF
int snprintf(char *str, size_t n, const char *fmt, ...);
#endif

namespace KTools {
	enum ExitStatus {
		MagickErrorCode = 1,
		KTEXErrorCode = 2,
		GeneralErrorCode = -1
	};

	class Error : public std::exception
	{
	public:
		Error( const std::string& cppwhat_ ) : cppwhat( cppwhat_ ) {}
		virtual ~Error() throw() {}
	
		virtual char const* what() const throw() { return cppwhat.c_str(); }
	
	private:
		std::string cppwhat;
	};

	class KToolsError : public Error {
	public:
		KToolsError(const std::string& kuwhat) : Error("Error: " + kuwhat) {}
		virtual ~KToolsError() throw() {}
	};


	class NonCopyable
	{
	public:
	        NonCopyable() {}
	       
	private:
	        NonCopyable( NonCopyable const& );
	        NonCopyable& operator=( NonCopyable const& );
	};


	// For initializer lists.
	template<typename T, typename U>
	struct raw_pair {
		T first;
		U second;
	};

	static const class Nil {} nil;

	template<typename T> class Maybe;

	template<typename T> Maybe<T> Just(T val);

	template<typename T>
	class Maybe {
		template<typename> friend class Maybe;
		friend Maybe<T> Just<T>(T);

		bool is_nothing;
		T val;

		Maybe(T _val) : is_nothing(false), val(_val) {}

	public:
		Maybe& operator=(const Maybe& m) {
			is_nothing = m.is_nothing;
			val = m.val;
			return *this;
		}

		Maybe() : is_nothing(true) {}
		Maybe(Nil) : is_nothing(true) {}
		Maybe(const Maybe& m) { *this = m; }

		template<typename U>
		bool operator==(const Maybe<U>& m) const {
			return is_nothing && m.is_nothing;
		}

		bool operator==(Nil) const {
			return is_nothing;
		}

		friend bool operator==(Nil, const Maybe& m) {
			return m.is_nothing;
		}

		bool operator==(const Maybe& m) const {
			return (is_nothing && m.is_nothing) || (!is_nothing && !m.is_nothing && val == m.val);
		}

		template<typename U>
		bool operator!=(const U& u) const {
			return !(*this == u);
		}

		T value() const {
			if(is_nothing) {
				throw Error("Attempt to cast nil to a value.");
			}
			return val;
		}

		operator T() const {
			return value();
		}
	};

	template<typename T>
	inline Maybe<T> Just(T val) {
		return Maybe<T>(val);
	}

	int strformat(std::string& s, const char* fmt, ...);
}

#endif
