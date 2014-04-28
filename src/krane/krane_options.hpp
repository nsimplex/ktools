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


#ifndef KRANE_OPTIONS_HPP
#define KRANE_OPTIONS_HPP

#include "krane_common.hpp"

namespace Krane {
	namespace options {
		extern Maybe<std::string> allowed_build;

		typedef std::vector<std::string> allowed_banks_t;
		extern allowed_banks_t allowed_banks;

		extern Maybe<std::string> build_rename;

		extern Maybe<std::string> banks_rename;

		extern bool check_animation_fidelity;

		extern bool mark_atlas;

		extern int verbosity;
		extern bool info;
	}
}

#endif
