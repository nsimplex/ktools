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


#ifndef KTOOLS_KTEX_HEADERFIELDSPECS_HPP
#define KTOOLS_KTEX_HEADERFIELDSPECS_HPP

#include "ktools_common.hpp"

namespace KTools {
	namespace KTEX {
		struct HeaderFieldSpec {
			typedef uint32_t value_t;

			static const HeaderFieldSpec Invalid;

			// Numerical identifier (order, counting from 0);
			size_t idx;

			// String identifier.
			std::string id;

			bool isValid() const {
				return id.length() > 0;
			}

			// Human readable name.
			std::string name;

			// Length, in bits.
			unsigned int length;

			// Offset, in bits (relative to the end of the magic number).
			unsigned int offset;

			typedef std::map<std::string, value_t> values_map_t;
			typedef std::map<value_t, std::string> values_inverse_map_t;

			values_map_t values;
			values_inverse_map_t values_inverse;
			
			value_t value_default;


			value_t normalize_value(value_t v) const {
				return v;
			}

			value_t normalize_value(const std::string& s) const {
				std::map<std::string, value_t>::const_iterator it = values.find(s);
				if(it == values.end()) {
					return value_default;
				}
				else {
					return it->second;
				}
			}

			const std::string& normalize_value_inverse(const std::string& s) const {
				return s;
			}

			const std::string& normalize_value_inverse(value_t v) const {
				static const std::string emptystring = "";

				std::map<value_t, std::string>::const_iterator it = values_inverse.find(v);
				if(it == values_inverse.end()) {
					return emptystring;
				} else {
					return it->second;
				}
			}

			operator bool() const {
				return this != &Invalid;
			}


			HeaderFieldSpec() {}

		private:
			void ConstructBasic(const std::string& _id, const std::string& _name, unsigned int _length, value_t _value_default) {
				id = _id;
				name = _name;
				length = _length;
				value_default = _value_default;
			}

			void ConstructValues(const raw_pair<std::string, value_t>* _value_pairs, size_t nvalues) {
				for(size_t i = 0; i < nvalues; i++) {
					values.insert(std::make_pair(_value_pairs[i].first, _value_pairs[i].second));
					values_inverse.insert(std::make_pair(_value_pairs[i].second, _value_pairs[i].first));
				}
			}


			void Construct(const std::string& _id, const std::string& _name, unsigned int _length, const raw_pair<std::string, value_t>* _value_pairs, size_t nvalues, value_t _value_default) {
				ConstructBasic(_id, _name, _length, _value_default);
				ConstructValues(_value_pairs, nvalues);
			}

			void Construct(const std::string& _id, const std::string& _name, unsigned int _length, const raw_pair<std::string, value_t>* _value_pairs, size_t nvalues, const std::string& value_default_name) {
				ConstructBasic(_id, _name, _length, value_t());
				ConstructValues(_value_pairs, nvalues);
				value_default = normalize_value(value_default_name);
			}


		public:
			HeaderFieldSpec(const std::string& _id, const std::string& _name, unsigned int _length, const raw_pair<std::string, value_t>* _value_pairs, size_t nvalues, value_t _value_default = value_t()) {
					Construct(_id, _name, _length, _value_pairs, nvalues, _value_default);
			}

			HeaderFieldSpec(const std::string& _id, const std::string& _name, unsigned int _length, const raw_pair<std::string, value_t>* _value_pairs, size_t nvalues, const std::string& value_default_name) {
					Construct(_id, _name, _length, _value_pairs, nvalues, value_default_name);
			}

			HeaderFieldSpec(const std::string& _id, const std::string& _name, unsigned int _length, value_t _value_default = value_t()) {
				ConstructBasic(_id, _name, _length, _value_default);
			}
		};
	}
}
#endif
