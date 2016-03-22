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

#ifdef NDEBUG
#	undef NDEBUG
#endif

#ifdef _MSC_VER
// Mostly to avoid warnings related to ImageMagick classes.
// This is why the pragma is done here, and not in compat/*.hpp.
#	pragma warning( disable: 4251 )
#	pragma warning( disable: 4275 )
#endif


#include "config.h"

#include "compat.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cerrno>

extern "C" {
#if defined(HAVE_STDINT_H)
#	include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#	include <inttypes.h>
#elif defined(HAVE_SYS_TYPES_H)
#	include <sys/types.h>
#endif
}

#ifdef HAVE_CSTDDEF
#	include <cstddef>
#else
extern "C" {
#	include <stddef.h>
}
#endif

#include <exception>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <stack>
#include <queue>
#include <set>
#include <map>
#include <iterator>
#include <algorithm>
#include <functional>


#include <Magick++.h>


#ifndef HAVE_SNPRINTF
#	ifdef HAVE__SNPRINTF
#		define snprintf _snprintf
#		define HAVE_SNPRINTF 1
#	else
int snprintf(char *str, size_t n, const char *fmt, ...) PRINTFSTYLE(3, 4);
#	endif
#endif

#ifndef RESTRICT
#	if defined(HAVE_RESTRICT)
#		define RESTRICT restrict
#	elif defined(HAVE___RESTRICT)
#		define RESTRICT __restrict
#	elif defined(HAVE___RESTRICT__)
#		define RESTRICT __restrict__
#	else
#		define RESTRICT
#	endif
#endif


namespace KTools {
	void initialize_application(int& argc, char **& argv);


	typedef double float_type;

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

	protected:
		void setMessage(const std::string& cppwhat_) {
			cppwhat = cppwhat_;
		}

		Error() : cppwhat() {}
	
	private:
		std::string cppwhat;
	};

	class KToolsError : public Error {
	public:
		KToolsError(const std::string& kuwhat) : Error(kuwhat) {}
		virtual ~KToolsError() throw() {}
	};

	class EncodingVersionError : public KToolsError {
	public:
		EncodingVersionError(const std::string& _what) : KToolsError(_what) {}
	};

	class SysError : public Error {
	public:
		SysError(const std::string& _what = "") : Error() {
			std::string msg = _what;
			if(errno != 0) {
				if(msg.length() > 0) {
					msg.append(": ");
				}
				msg.append(strerror(errno));
			}
			setMessage(msg);
		}
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

	class lcstring {
	public:
		const char * const data;
		const size_t len;

		lcstring(const char *s) : data(s), len(::strlen(s)) {}
		lcstring(const char *s, size_t l) : data(s), len(l) {}
		lcstring(const std::string& s) : data(s.c_str()), len(s.length()) {}

		inline operator const char*() const {
			return data;
		}

		inline const char* operator()(void) const {
			return data;
		}

		inline size_t length() const {
			return len;
		}

		inline size_t size() const {
			return len;
		}

		inline std::string tostring() const {
			return std::string(data, len);
		}
	};


	template<typename T, typename U>
	inline T& cast_assign(T& lval, U rval) {
		lval = static_cast<T>(rval);
		return lval;
	}


	template<typename T>
	class ptrLess : public std::binary_function<T*, T*, bool> {
	public:
		bool operator()(const T* a, const T* b) const {
			return *a < *b;
		}
	};


	static const class Nil { public: Nil() {} } nil;

	static inline bool operator==(Magick::Image img, Nil) {
		if(img.columns() == 0) {
			assert( img.rows() == 0 );
			return true;
		}
		else {
			assert( img.rows() > 0 );
			return false;
		}
	}


	template<typename T> class Maybe;

	template<typename T> inline Maybe<T> Just(T val);

	template<typename T>
	class Maybe {
		template<typename> friend class Maybe;
		friend Maybe<T> Just<T>(T);

		bool is_nothing;
		T v;

		explicit Maybe(T _v) : is_nothing(false), v(_v) {}

	public:
		typedef T value_type;

		Maybe& operator=(const Maybe& m) {
			is_nothing = m.is_nothing;
			v = m.v;
			return *this;
		}

		Maybe& operator=(Nil) {
			is_nothing = true;
			return *this;
		}

		Maybe() : is_nothing(true), v() {}
		Maybe(Nil) : is_nothing(true), v() {}
		Maybe(const Maybe& m) { *this = m; }

		bool operator==(Nil) const {
			return is_nothing;
		}

		friend inline bool operator==(Nil, const Maybe& m) {
			return m.is_nothing;
		}

		bool operator==(const Maybe& m) const {
			return (is_nothing && m.is_nothing) || (!is_nothing && !m.is_nothing && v == m.v);
		}

		template<typename U>
		bool operator==(const U& u) const {
			return !is_nothing && v == u;
		}

		template<typename U>
		friend bool operator==(const U& u, const Maybe& m) {
			return m == u;
		}

		template<typename U>
		bool operator==(const Maybe<U>& m) const {
			return is_nothing && m.is_nothing;
		}

		template<typename U>
		bool operator!=(const U& u) const {
			return !(*this == u);
		}

		template<typename U>
		U to() {
			if(is_nothing) {
				throw Error("Attempt to cast nil to a value.");
			}
			return static_cast<U>(v);
		}

		template<typename U>
		U to() const {
			if(is_nothing) {
				throw Error("Attempt to cast nil to a value.");
			}
			return static_cast<U>(v);
		}

		T value() const {
			return to<T>();
		}

		T val() const {
			return to<T>();
		}

		T& ref() {
			return to<T&>();
		}

		const T& ref() const {
			return to<const T&>();
		}

		operator T() const {
			return value();
		}
	};

	template<typename T>
	Maybe<T> Just(T val) {
		return Maybe<T>(val);
	}

	int strformat_inplace(std::string& s, const char* fmt, ...) PRINTFSTYLE(2, 3);

	std::string strformat(const char* fmt, ...) PRINTFSTYLE(1, 2);

	/*
	 * Each call destroys the validity of the last return.
	 * (the buffer is reused)
	 */
	class DataFormatter {
		mutable char buffer[1 << 15];

	public:
		const char * operator()(const char * fmt, ...) const;
	};

	template<typename charT, typename charTraits>
	inline bool check_basic_stream_validity(std::basic_ios<charT, charTraits>& file, const std::string& prefix, bool _throw) {
		if(file.fail()) {
			if(_throw) {
				throw SysError(prefix);
			}
			return false;
		}
		return true;
	}

	inline bool check_stream_validity(std::istream& in, const std::string& fname = "file", bool _throw = true) {
		(void)in.peek();
		return check_basic_stream_validity(in, "Failed to open " + fname + " for reading", _throw);
	}

	inline bool check_stream_validity(std::ostream& out, const std::string& fname = "file", bool _throw = true) {
		return check_basic_stream_validity(out, "Failed to open " + fname + " for writing", _throw);
	}
}


#define MAGICK_WRAP(code) try {\
	code ; \
} \
catch(Magick::WarningCoder& warning) { \
	std::cerr << "Coder warning: " << warning.what() << std::endl; \
} \
catch(Magick::Warning& warning) { \
	std::cerr << "Warning: " << warning.what() << std::endl; \
} \
catch(Magick::Error& err) { \
	cerr << "Error: " << err.what() << endl; \
	exit(MagickErrorCode); \
}


#endif
