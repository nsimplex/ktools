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


#ifndef KRANE_KBUILD_HPP
#define KRANE_KBUILD_HPP

#include "krane_common.hpp"
#include "algebra.hpp"


namespace Krane {
	class KBuild : public NonCopyable {
		static const uint32_t MAGIC_NUMBER;

		int32_t version;
		uint32_t numsymbols;
		uint32_t numframes;

		std::string name;

	public:
		class Symbol {
			friend class KBuild;

			std::string name;
			hash_t hash;

			class Frame {
				friend class Symbol;
				friend class KBuild;

				uint32_t framenum;
				uint32_t duration;

				float x, y;
				float w, h;

				uint32_t alphaidx;
				uint32_t alphacount;
			};

			std::vector<Frame> frames;
		};

	private:
		std::vector<std::string> atlasnames;

		std::vector<Symbol> symbols;

		// alphaverts
	};
}


#endif
