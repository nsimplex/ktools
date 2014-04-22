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

				/*
				 * Updates the atlas bounding box based on the uvw triangles.
				 */
				void updateAtlasBoundingBox() {
					atlas_bbox.reset();

					size_t ntrigs = uvwtriangles.size();
					for(size_t i = 0; i < ntrigs; i++) {
						atlas_bbox.addPoints(uvwtriangles[i]);
					}

					cropAtlasBoundingBox();
				}

				void cropAtlasBoundingBox() {
					atlas_bbox.intersect(bbox_type(0, 0, 1, 1));
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
					cropAtlasBoundingBox();
				}

				uint32_t countTriangles() const {
					return uint32_t(xyztriangles.size());
				}

				uint32_t countAlphaVerts() const {
					return 3*countTriangles();
				}

				Magick::Image getImage() const {
					using namespace Magick;
					using namespace std;

					// Bounding quad.
					Image quad;
					// Size of the atlas.
					float_type w0, h0;
					bbox_type::vector_type offset;
					{
						Image atlas = parent->parent->getFrontAtlas();
						Geometry cropgeo = atlas.size();

						w0 = cropgeo.width();
						h0 = cropgeo.height();

						offset[0] = w0*atlas_bbox.x();
						offset[1] = h0*(1 - atlas_bbox.ymax());

						cropgeo.xOff(size_t( floor(offset[0]) ));
						cropgeo.yOff(size_t( floor(offset[1]) ));
						cropgeo.width(size_t( floor(w0*atlas_bbox.w()) ));
						cropgeo.height(size_t( floor(h0*atlas_bbox.h()) ));

						quad = atlas;
						quad.crop(cropgeo);
					}


					Geometry geo = quad.size();

					// Returned image (clipped quad).
					Image img = Image(geo, "transparent");

					// Clip mask.
					Image mask(geo, Color("white"));
					mask.fillColor("black");

					list<Drawable> drawable_trigs;

					size_t ntrigs = uvwtriangles.size();
					for(size_t i = 0; i < ntrigs; i++) {
						triangle_type trig = uvwtriangles[i];

						list<Coordinate> coords;

						coords.push_back( Coordinate(trig.a[0]*w0 - offset[0], (1 - trig.a[1])*h0 - offset[1]) );
						coords.push_back( Coordinate(trig.b[0]*w0 - offset[0], (1 - trig.b[1])*h0 - offset[1]) );
						coords.push_back( Coordinate(trig.c[0]*w0 - offset[0], (1 - trig.c[1])*h0 - offset[1]) );

						drawable_trigs.push_back( DrawablePolygon(coords) );
					}

					mask.draw(drawable_trigs);

					img.clipMask(mask);

					img.composite( quad, Geometry(0, 0), OverCompositeOp );

					img.magick("RGBA");

					return img;
				}

				void getName(std::string& s) const {
					std::string parent_name;
					parent->getName(parent_name);
					strformat(s, "%s-%u", parent_name.c_str(), (unsigned int)framenum);
				}

				std::string getName() const {
					std::string s;
					getName(s);
					return s;
				}

				void getPath(Compat::Path& p) const {
					parent->getPath(p);
				
					Compat::Path suffix;
					getName(suffix);

					p /= suffix;
				}

				Compat::Path getPath() const {
					Compat::Path p;
					getPath(p);
					return p;
				}

				std::istream& loadPre(std::istream& in, int verbosity);
				std::istream& loadPost(std::istream& in, int verbosity);
			};

		private:
			std::string name;
			hash_t hash;

			void setHash(hash_t h) {
				hash = h;
			}

		public:
			typedef std::vector<Frame> framelist_t;
			framelist_t frames;

			bool operator<(const Symbol& s) const {
				return hash < s.hash || (hash == s.hash && name < s.name);
			}

			bool operator==(const Symbol& s) const {
				return hash == s.hash && name == s.name;
			}

			bool operator!=(const Symbol& s) const {
				return !(*this == s);
			}

			void getName(std::string& s) const {
				s = name;
			}

			const std::string& getName() const {
				return name;
			}

			void getPath(Compat::Path& p) const {
				p.assign(getName());
			}

			Compat::Path getPath() const {
				Compat::Path p;
				getPath(p);
				return p;
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

		typedef Magick::Image atlas_t;

	private:
		std::string name;

		std::vector<std::string> atlasnames;
		std::vector<atlas_t> atlases;

		typedef std::map<hash_t, Symbol> symbolmap_t;

	public:
		symbolmap_t symbols;


		const std::string& getName() const {
			return name;
		}

		void setName(const std::string& s) {
			name = s;
		}

		const std::string& getFrontAtlasName() const {
			if(atlasnames.empty()) {
				throw(std::logic_error("No atlases."));
			}
			return atlasnames[0];
		}

		atlas_t& getFrontAtlas() {
			if(atlases.empty()) {
				throw(std::logic_error("No atlases."));
			}
			return atlases[0];
		}

		const atlas_t& getFrontAtlas() const {
			if(atlases.empty()) {
				throw(std::logic_error("No atlases."));
			}
			return atlases[0];
		}

		void setFrontAtlas(const atlas_t& a) {
			if(atlases.empty()) {
				atlases.resize(1);
			}
			atlases[0] = a;
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

		template<typename OutputIterator>
		void getImages(OutputIterator it) const {
			typedef symbolmap_t::const_iterator symbol_iter;
			typedef Symbol::framelist_t::const_iterator frame_iter;

			for(symbol_iter symit = symbols.begin(); symit != symbols.end(); ++symit) {
				for(frame_iter frit = symit->second.frames.begin(); frit != symit->second.frames.end(); ++frit) {
					*it++ = std::make_pair(frit->getPath(), frit->getImage());
				}
			}
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
		KBuild build;

	public:
		BinIOHelper io;

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

		KBuild& getBuild() {
			return build;
		}

		const KBuild& getBuild() const {
			return build;
		}

		std::istream& load(std::istream& in, int verbosity);
		void loadFrom(const std::string& path, int verbosity);

		KBuildFile() : version(MAX_VERSION) {}
	};
}


#endif
