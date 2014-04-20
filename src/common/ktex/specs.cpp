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


#include "ktools_common.hpp"
#include "ktex/specs.hpp"

using namespace KTools;
using namespace KTools::KTEX;


const KTools::KTEX::HeaderFieldSpec KTools::KTEX::HeaderFieldSpec::Invalid;


#define ARRAYLEN(A) sizeof(A)/sizeof(*A)

typedef KTools::raw_pair<std::string, KTools::KTEX::HeaderFieldSpec::value_t> valpair_t;


const uint32_t KTools::KTEX::HeaderSpecs::MAGIC_NUMBER = *reinterpret_cast<const uint32_t*>("KTEX");


static const valpair_t platform_values[] = {
	{"Default", 0},
	{"PC", 12},
	{"PS3", 10},
	{"Xbox 360", 11}
};

static const valpair_t compression_values[] = {
	{"DXT1", 0},
	{"DXT3", 1},
	{"DXT5", 2},
	{"RGBA", 4},
	{"RGB", 5}

	/*
	{"atitc", 8},
	{"atitc_a_e", 9},
	{"atitc_a_i", 10},
	*/
};

static const valpair_t texturetype_values[] = {
	{"1D", 0},
	{"2D", 1},
	{"3D", 2},
	{"Cube Mapped", 3}
};

/*
 * Lists the header field specs.
 *
 * In the long constructor, we list:
 * string id, human readable name, length (bits), value pairs array, value pairs array length, default value (either numerical or in string form).
 *
 * In the short constructor, we list:
 * string id, human readable name, length (bits), default value.
 *
 * The default value is optional in both, defaulting to 0 (the result of the value type's standard constructor).
 */
static HeaderFieldSpec fieldspecs[] = {
	HeaderFieldSpec( "platform", "Platform", 4, platform_values, ARRAYLEN(platform_values), "DEFAULT" ),
	HeaderFieldSpec( "compression", "Compression Type", 5, compression_values, ARRAYLEN(compression_values), "DXT5" ),
	HeaderFieldSpec( "texture_type", "Texture Type", 4, texturetype_values, ARRAYLEN(texturetype_values), "2D" ),
	HeaderFieldSpec( "mipmap_count", "Mipmap Count", 5 ),
	HeaderFieldSpec( "flags", "Flags", 2, (1 << 2) - 1 ),

	HeaderFieldSpec( "fill", "Fill", 12, (1 << 12) - 1 )
};


const KTools::KTEX::HeaderSpecs::FieldSpecsMap_t KTools::KTEX::HeaderSpecs::FieldSpecs(fieldspecs, ARRAYLEN(fieldspecs));

/*
// The specs table should be at the stack top.
void KTools::KTEX::process_header_specs_table(lua_State* L) {
	const int top = lua_gettop(L);

	assert( lua_type(L, -1) == LUA_TTABLE );
	{
		size_t n = K_LUA_LEN(L, -1);

		if(n == 0) {
			luaL_error(L, "Empty header spec table provided!");
		}

		HeaderSpecs::FieldSpecs.resize(n);

		unsigned int offset = 0;

		for(size_t i = 1; i <= n; ++i) {
			lua_rawgeti(L, -1, i);
			{
				HeaderFieldSpec& spec = HeaderSpecs::FieldSpecs[i - 1];

				spec.idx = i - 1;

				lua_getfield(L, -1, "id");
				spec.id = luaL_checkstring(L, -1);
				lua_pop(L, 1);

				HeaderSpecs::FieldIdMap[spec.id] = i - 1;

				lua_getfield(L, -1, "name");
				spec.name = luaL_checkstring(L, -1);
				lua_pop(L, 1);
				
				lua_getfield(L, -1, "length");
				spec.length = luaL_checkinteger(L, -1);
				lua_pop(L, 1);

				spec.offset = offset;
				offset += spec.length;

				lua_getfield(L, -1, "values");
				if(lua_type(L, -1) == LUA_TTABLE) {
					std::string default_key;

					lua_pushnil(L);
					while(lua_next(L, -2)) {
						if(lua_type(L, -2) == LUA_TSTRING) {
							const char * k = lua_tolstring(L, -2, NULL);
							HeaderFieldSpec::value_t v = static_cast<HeaderFieldSpec::value_t>( luaL_checkinteger(L, -1) );

							spec.values[k] = v;
							spec.values_inverse[v] = k;
						} else if(lua_type(L, -2) == LUA_TNUMBER && lua_tointeger(L, -2) == 1) {
							default_key = luaL_checkstring(L, -1);
						}

						lua_pop(L, 1);
					}

					if( spec.values.find(default_key) == spec.values.end() ) {
						luaL_error(L, "Default value not specified for `%s'.", spec.id.c_str());
					}
				} else if(!lua_isnil(L, -1)) {
					luaL_error(L, "Table or nil expected as a `values' entry for `%s'.", spec.id.c_str());
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}
	}

	assert( lua_gettop(L) == top );
}
*/
