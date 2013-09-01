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


#ifndef KTECH_BASIC_HPP
#define KTECH_BASIC_HPP


#include <config.h>


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
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include <Magick++.h>

#include <squish/squish.h>

namespace KTech {
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

	class KleiUtilsError : public Error {
	public:
		KleiUtilsError(const std::string& kuwhat) : Error("KTech error: " + kuwhat) {}
		virtual ~KleiUtilsError() throw() {}
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
}

#endif
