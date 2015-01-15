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


#ifndef KTOOLS_KTEX_SPECS_HPP
#define KTOOLS_KTEX_SPECS_HPP

#include "ktex/headerfield_specs.hpp"

namespace KTools {
	namespace KTEX {
		struct HeaderSpecs {
			static const uint32_t MAGIC_NUMBER;

			static const size_t DATASIZE = 4;
			static const size_t TOTALSIZE = 8;

			static const class FieldSpecsMap_t {
				typedef std::map<std::string, HeaderFieldSpec> map_t;
				map_t M;

				// Ids sorted by the corresponding
				// HeaderFieldSpec's offset.
				typedef std::vector<std::string> sorted_ids_t;
				sorted_ids_t sorted_ids;
			public:
				typedef map_t::const_iterator const_iterator;
				// It's always const.
				typedef const_iterator iterator;

				size_t size() const {
					return M.size();
				}

				FieldSpecsMap_t(HeaderFieldSpec* A, size_t length) {
					unsigned int offset = 0;
					for(size_t i = 0; i < length; i++) {
						A[i].offset = offset;
						offset += A[i].length;

						M.insert( std::make_pair(A[i].id, A[i]) );
						sorted_ids.push_back(A[i].id);
					}
				}

				const_iterator begin() const {
					return M.begin();
				}

				const_iterator end() const {
					return M.end();
				}

				const_iterator find(const std::string& k) const {
					return M.find(k);
				}

				const HeaderFieldSpec& operator[](const std::string& k) const {
					const_iterator it = M.find(k);
					if(it != M.end()) {
						return it->second;
					}
					else {
						return HeaderFieldSpec::Invalid;
					}
				}

				class offset_iterator : public sorted_ids_t::const_iterator {
					friend class FieldSpecsMap_t;

					typedef sorted_ids_t::const_iterator SUPER;

					const FieldSpecsMap_t* map;

					offset_iterator(const FieldSpecsMap_t* _map, const SUPER& _base) : SUPER(_base), map(_map) {}
				public:
					offset_iterator() : map(NULL) {}

					const map_t::value_type& operator*() const {
						if(map == NULL) {
							throw Error("attempt to dereference an invalid iterator");
						}
						return *map->find(SUPER::operator*());
					}

					const map_t::value_type* operator->() const {
						if(map == NULL) {
							throw Error("attempt to dereference an invalid iterator");
						}
						return &*map->find(SUPER::operator*());
					}

					offset_iterator& operator++() {
						SUPER::operator++();
						return *this;
					}

					offset_iterator operator++(int) {
						offset_iterator dup(*this);
						++(*this);
						return dup;
					}

					bool operator==(const offset_iterator& it) const {
						return *static_cast<const SUPER*>(this) == *static_cast<const SUPER*>(&it);
					}

					bool operator!=(const offset_iterator& it) const {
						return !(*this == it);
					}
				};

				offset_iterator offset_begin() const {
					return offset_iterator(this, sorted_ids.begin());
				}

				offset_iterator offset_end() const {
					return offset_iterator(this, sorted_ids.end());
				}
			} FieldSpecs;

			typedef FieldSpecsMap_t::const_iterator field_spec_iterator;
			typedef FieldSpecsMap_t::offset_iterator field_spec_offset_iterator;
		};
	}
}

#endif
