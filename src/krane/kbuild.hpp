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
#include "binary_io_utils.hpp"


namespace Krane {
	class KBuildFile;

	class KBuild : public NestedSerializer<KBuildFile> {
		friend class KBuildFile;
	public:
		typedef computations_float_type float_type;

		class Symbol : public NestedSerializer<KBuild> {
			friend class KBuild;
		public:
			class Frame : public NestedSerializer<Symbol> {
				friend class Symbol;
				friend class KBuild;

				typedef Triangle<float_type> triangle_type;
				typedef BoundingBox<float_type> bbox_type;

				uint32_t framenum;
				uint32_t duration;

				bbox_type bbox;

				std::vector<triangle_type> xyztriangles;
				std::vector<triangle_type> uvwtriangles;

				void setAlphaVertCount(uint32_t n) {
					const uint32_t trigs = n/3;
					xyztriangles.resize(trigs);
					uvwtriangles.resize(trigs);
				}

			public:
				/*
				 * Bounding box of the corresponding atlas region, in UV coordinates.
				 */
				BoundingBox<float_type> atlas_bbox;

				void addTriangles(const triangle_type& xyz, const triangle_type& uvw) {
					xyztriangles.push_back(xyz);
					uvwtriangles.push_back(uvw);
					atlas_bbox.addPoints(uvw);
				}

				uint32_t countTriangles() const {
					return uint32_t(xyztriangles.size());
				}

				uint32_t countAlphaVerts() const {
					return 3*countTriangles();
				}

				std::istream& loadPre(std::istream& in, int verbosity);
				std::istream& loadPost(std::istream& in, int verbosity);
			};

		private:
			std::string name;
			hash_t hash;

			std::vector<Frame> frames;

			void setHash(hash_t h) {
				hash = h;
			}

		public:
			bool operator<(const Symbol& s) const {
				return hash < s.hash || (hash == s.hash && name < s.name);
			}

			bool operator==(const Symbol& s) const {
				return hash == s.hash && name == s.name;
			}

			bool operator!=(const Symbol& s) const {
				return !(*this == s);
			}

			const std::string& getName() const {
				return name;
			}

			operator const std::string&() const {
				return name;
			}

			uint32_t countAlphaVerts() const {
				uint32_t count = 0;

				const size_t nframes = frames.size();
				for(size_t i = 0; i < nframes; i++) {
					count += frames[i].countAlphaVerts();
				}

				return count;
			}

			std::istream& loadPre(std::istream& in, int verbosity);
			std::istream& loadPost(std::istream& in, int verbosity);
		};

	private:
		std::string name;

		std::vector<std::string> atlasnames;

		typedef std::map<hash_t, Symbol> symbolmap_t;
		symbolmap_t symbols;

	public:
		const std::string& getName() const {
			return name;
		}

		void setName(const std::string& s) {
			name = s;
		}

		Symbol* getSymbol(hash_t h) {
			symbolmap_t::iterator it = symbols.find(h);
			if(it == symbols.end()) {
				return NULL;
			}
			else {
				return &(it->second);
			}
		}
		
		const Symbol* getSymbol(hash_t h) const {
			symbolmap_t::const_iterator it = symbols.find(h);
			if(it == symbols.end()) {
				return NULL;
			}
			else {
				return &(it->second);
			}
		}

		Symbol* getSymbol(const std::string& symname) {
			return getSymbol(strhash(symname));
		}

		const Symbol* getSymbol(const std::string& symname) const {
			return getSymbol(strhash(symname));
		}

		std::istream& load(std::istream& in, int verbosity);
	};


	class KBuildFile : public NonCopyable {
	public:
		static const uint32_t MAGIC_NUMBER;

		static const int32_t MIN_VERSION;
		static const int32_t MAX_VERSION;

	private:
		int32_t version;

	public:
		BinIOHelper io;
		KBuild build;

		static bool checkVersion(int32_t v) {
			return MIN_VERSION <= v && v <= MAX_VERSION;
		}

		bool checkVersion() const {
			return checkVersion(version);
		}

		void versionRequire() const {
			if(!checkVersion()) {
				std::cerr << "Build has unsupported encoding version " << version << "." << std::endl;
				throw(KToolsError("Build has unsupported encoding version."));
			}
		}

		int32_t getVersion() const {
			return version;
		}

		std::istream& load(std::istream& in, int verbosity);
		void loadFrom(const std::string& path, int verbosity);

		KBuildFile() : version(MAX_VERSION) {}
	};
}


#endif
