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


#ifndef KRANE_BASIC_HPP
#define KRANE_BASIC_HPP

#include "ktools_common.hpp"

namespace Krane {
	using namespace KTools;

	typedef uint32_t hash_t;
	typedef int32_t signed_hash_t;

	inline hash_t strhash(const std::string& str) {
		const size_t len = str.length();
		signed_hash_t h = 0;
		for(size_t i = 0; i < len; i++) {
			signed_hash_t c = tolower(str[i]);
			h = (c + (h << 6) + (h << 16) - h); // & 0xffffffff;
		}
		return *reinterpret_cast<hash_t*>(&h);
	}
}

#endif
