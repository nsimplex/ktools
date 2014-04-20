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


#ifndef KTOOLS_BIT_OP_HPP
#define KTOOLS_BIT_OP_HPP

#include "ktools_common.hpp"
#include <limits>


namespace KTech { namespace BitOp {
	class Pow2Rounder {
		template<typename IntegerType>
		class Metadata {
		public:
			static const IntegerType max_pow_2;

			/*
			 * Returns true for 0. For other values, works correctly.
			 */
			static bool isPow2(IntegerType n) {
				return !(n & (n-1));
			}
		};

	public:
		/*
		 * If n == 0, returns 0.
		 */
		template<typename IntegerType>
		static IntegerType roundDown(IntegerType n) {
			/*
			 * The number of loops is equal to the number of non-zero bits
			 * in n other than the highest bit.
			 */
			while(!Metadata<IntegerType>::isPow2(n)) {
				n = n & (n-1);
			}

			return n;
		}

		template<typename IntegerType>
		static IntegerType roundUp(IntegerType n) {
			if(Metadata<IntegerType>::isPow2(n)) {
				return n;
			}

			n = roundDown(n);

			if(n < Metadata<IntegerType>::max_pow_2) {
				n <<= 1;
			}

			return n;
		}
	};

	template<typename IntegerType>
	inline size_t countBinaryDigits(IntegerType n) {
		size_t ret = 0;
		while(n) {
			n >>= 1;
			ret++;
		}
		return ret;
	}
}}

#endif
