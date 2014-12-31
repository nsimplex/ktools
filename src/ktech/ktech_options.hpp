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


#ifndef KTECH_OPTIONS_HPP
#define KTECH_OPTIONS_HPP

#include "ktech_common.hpp"
#include "file_abstraction.hpp"

namespace KTech {
	namespace options {
		extern int verbosity;

		extern bool info;

		extern int image_quality;

		extern Magick::FilterTypes filter;

		extern bool no_premultiply;

		extern bool no_mipmaps;

		extern Maybe<size_t> width;
		extern Maybe<size_t> height;
		extern bool pow2;

		extern bool force_square;
		extern bool extend;
		extern bool extend_left;

		extern Maybe<VirtualPath> atlas_path;
	}
}

#endif
