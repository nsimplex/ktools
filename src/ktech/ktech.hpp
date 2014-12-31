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


#ifndef KTECH_HPP
#define KTECH_HPP

#include "ktech_common.hpp"
#include "ktools_bit_op.hpp"
#include "ktech_options.hpp"
#include "ktex/ktex.hpp"
#include "file_abstraction.hpp"

namespace KTech {
	KTEX::File::Header parse_commandline_options(int& argc, char**& argv, std::list<VirtualPath>& input_paths, VirtualPath& output_path);
}

#endif
