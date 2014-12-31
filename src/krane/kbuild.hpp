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
		

		// This is the scale applied by the scml compiler in the mod tools.
		static const float_type MODTOOLS_SCALE;

		class Symbol : public NestedSerializer<KBuild> {
			friend class KBuild;
		public:
			class Frame : public NestedSerializer<Symbol> {
				friend class Symbol;
				friend class KBuild;

			public:
				typedef Triangle<float_type, 3> xyztriangle_type;
				typedef Triangle<float_type, 2> uvwtriangle_type;
				typedef BoundingBox<float_type> bbox_type;

			private:
				std::vector<xyztriangle_type> xyztriangles;
				std::vector<uvwtriangle_type> uvwtriangles;

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

				/*
				 * This is the sum of the durations of frames coming before.
				 *
				 * Counts from 0.
				 */
				uint32_t framenum;

				/*
				 * This is the time length of this frame measured in animation frames.
				 */
				uint32_t duration;

				/*
				 * Corresponds to the x, y, w and h values of the xml.
				 */
				bbox_type bbox;

				/*
				 * "Depth" of the atlas (z coordinate of the uvw points).
				 */
				int atlas_depth;

				int getAtlasDepth() const {
					return atlas_depth;
				}

			public:
				uint32_t getFrameNumber() const {
					return framenum;
				}

				uint32_t getDuration() const {
					return duration;
				}

				bbox_type& getBoundingBox() {
					return bbox;
				}

				const bbox_type& getBoundingBox() const {
					return bbox;
				}

				int getAtlasIdx() const {
					return getAtlasDepth() - parent->getSamplerOffset();
				}
			
				/*
				 * Bounding box of the corresponding atlas region, in UV coordinates.
				 */
				BoundingBox<float_type> atlas_bbox;

				void addTriangles(const xyztriangle_type& xyz, const uvwtriangle_type& uvw) {
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

				void getGeometry(Magick::Geometry& geo) const;

				Magick::Geometry getGeometry() const {
					Magick::Geometry geo;
					getGeometry(geo);
					return geo;
				}

				/*
				 * Returns the clipping mask as a monochrome image with black
				 * on the region allowed to pass through.
				 *
				 * The image has the size of the frame's bounding box.
				 */
				Magick::Image getClipMask() const HOTFUNCTION;

				Magick::Image getImage() const HOTFUNCTION;

				void getName(std::string& s) const {
					std::string parent_name;
					parent->getName(parent_name);
					strformat_inplace(s, "%s-%u", parent_name.c_str(), (unsigned int)framenum);
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
					p += ".png";
				}

				Compat::Path getPath() const {
					Compat::Path p;
					getPath(p);
					return p;
				}

				void getUnixPath(Compat::UnixPath& p) const {
					parent->getUnixPath(p);
				
					Compat::UnixPath suffix;
					getName(suffix);

					p /= suffix;
					p += ".png";
				}

				Compat::UnixPath getUnixPath() const {
					Compat::UnixPath p;
					getUnixPath(p);
					return p;
				}

			private:
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
			typedef framelist_t::iterator frame_iterator;
			typedef framelist_t::const_iterator frame_const_iterator;

			framelist_t frames;

		private:
			// Maps frame numbers to indexes of the frames vector.
			typedef std::map<uint32_t, uint32_t> frameNumberMap_t;
			frameNumberMap_t frameNumberMap;

			void update_framenumbermap() {
				frameNumberMap.clear();
				
				const uint32_t nframes = uint32_t(frames.size());
				for(uint32_t i = 0; i < nframes; i++) {
					frameNumberMap.insert( std::make_pair(frames[i].getFrameNumber(), i) );
				}
			}

		public:
			void getName(std::string& s) const {
				s = name;
			}

			const std::string& getName() const {
				return name;
			}

			hash_t getHash() const {
				return hash;
			}

			int getSamplerOffset() const {
				return parent->getSamplerOffset();
			}

			/*
			 * Receives a frame number (as in the attribute frame number of Frame)
			 * and returns the index in the frames vector of the corresponding frame.
			 */
			uint32_t getFrameIndex(uint32_t framenum) const {
				if(frameNumberMap.empty()) {
					throw std::logic_error("querying an empty build symbol frame number map.");
				}
				frameNumberMap_t::const_iterator match = frameNumberMap.upper_bound(framenum);
				if(match == frameNumberMap.end()) {
					return frameNumberMap.rbegin()->second;
				}
				else {
					return (--match)->second;
				}
			}

			const Frame& getFrame(uint32_t framenum) const {
				return frames[getFrameIndex(framenum)];
			}

			Frame& getFrame(uint32_t framenum) {
				return frames[getFrameIndex(framenum)];
			}

			// This counts the number of symbol frames (i.e., distinct
			// animation states).
			uint32_t countFrames() const {
				return frames.size();
			}

			// This counts the number of animation frames (i.e., as a
			// measure of time).
			uint32_t countAnimationFrames() const {
				const size_t nsymframes = frames.size();
				if(nsymframes > 0) {
					const Frame& lastframe = frames[nsymframes - 1];
					return lastframe.framenum + lastframe.duration;
				}
				else {
					return 0;
				}
			}

			void getPath(Compat::Path& p) const {
				p.assign(getName());
			}

			Compat::Path getPath() const {
				Compat::Path p;
				getPath(p);
				return p;
			}

			void getUnixPath(Compat::UnixPath& p) const {
				p.assign(getName());
			}

			Compat::UnixPath getUnixPath() const {
				Compat::UnixPath p;
				getUnixPath(p);
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

		private:
			std::istream& loadPre(std::istream& in, int verbosity);
			std::istream& loadPost(std::istream& in, int verbosity);
		};

		typedef Magick::Image atlas_t;

		typedef Symbol::frame_iterator frame_iterator;
		typedef Symbol::frame_const_iterator frame_const_iterator;

	private:
		std::string name;

		/*
		 * Offset added to the atlas idx of each element frame to form
		 * the resulting atlas "depth".
		 */
		mutable Maybe<int> sampler_offset;

	public:
		typedef std::map<hash_t, Symbol> symbolmap_t;
		typedef symbolmap_t::iterator symbolmap_iterator;
		typedef symbolmap_t::const_iterator symbolmap_const_iterator;

		symbolmap_t symbols;

		typedef std::vector< std::pair<std::string, atlas_t> > atlaslist_t;
		atlaslist_t atlases;


		const std::string& getName() const {
			return name;
		}

		void setName(const std::string& s) {
			name = s;
		}

		/*
		const std::string& getFrontAtlasName() const {
			if(atlases.empty()) {
				throw(std::logic_error("Build has no atlases."));
			}
			return atlases[0].first;
		}

		atlas_t& getFrontAtlas() {
			if(atlases.empty()) {
				throw(std::logic_error("No atlases."));
			}
			return atlases[0].second;
		}

		const atlas_t& getFrontAtlas() const {
			if(atlases.empty()) {
				throw(std::logic_error("No atlases."));
			}
			return atlases[0].second;
		}

		void setFrontAtlas(const atlas_t& a) {
			if(atlases.empty()) {
				atlases.resize(1);
			}
			atlases[0].second = a;
		}
		*/

		int getSamplerOffset() const {
			if(sampler_offset == nil) {
				Maybe<int> max_depth;

				for(symbolmap_t::const_iterator symit = symbols.begin(); symit != symbols.end(); ++symit) {
					const Symbol::framelist_t& frames = symit->second.frames;
					for(Symbol::framelist_t::const_iterator fit = frames.begin(); fit != frames.end(); ++fit) {
						int depth = fit->getAtlasDepth();
						if(sampler_offset == nil || depth < sampler_offset) {
							sampler_offset = Just(depth);
						}
						if(max_depth == nil || depth > max_depth) {
							max_depth = Just(depth);
						}
					}
				}

				if(max_depth - sampler_offset + 1 > int(atlases.size())) {
					throw KToolsError("Build has symbol frames requesting atlases the build does not possess.");
				}
			}
			return sampler_offset;
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

		uint32_t countFrames() const {
			typedef symbolmap_t::const_iterator symbol_iter;

			uint32_t n = 0;
			for(symbol_iter it = symbols.begin(); it != symbols.end(); ++it) {
				n += it->second.countFrames();
			}
			return n;
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

		/*
		 * Clipping mask for the whole atlas with the given numerical id.
		 *
		 * Follows the same conventions as the symbol frame clipping mask.
		 */
		Magick::Image getClipMask(size_t idx) const HOTFUNCTION;

		// Gets all clip masks.
		template<typename OutputIterator>
		void getClipMasks(OutputIterator it) const {
			size_t idx = 0;
			for(atlaslist_t::const_iterator atit = atlases.begin(); atit != atlases.end(); ++atit) {
				*it++ = getClipMask(idx++);
			}
		}

		/*
		 * Returns the atlas with the given numerical id with its clipped off
		 * regions marked with the given color.
		 */
		Magick::Image getMarkedAtlas(size_t idx, Magick::Color c) const HOTFUNCTION;

		// Gets all marked atlases.
		template<typename OutputIterator>
		void getMarkedAtlases(OutputIterator it, Magick::Color c) const {
			size_t idx = 0;
			for(atlaslist_t::const_iterator atit = atlases.begin(); atit != atlases.end(); ++atit) {
				*it++ = std::make_pair( atit->first, getMarkedAtlas(idx++, c) );
			}
		}


	private:
		std::istream& load(std::istream& in, int verbosity);
	};


	class KBuildFile : public NonCopyable {
	public:
		static const uint32_t MAGIC_NUMBER;

		static const int32_t MIN_VERSION;
		static const int32_t MAX_VERSION;

	private:
		int32_t version;
		KBuild* build;

	public:
		BinIOHelper io;

		bool shouldHaveHashTable() const;

		static bool checkVersion(int32_t v) {
			return MIN_VERSION <= v && v <= MAX_VERSION;
		}

		bool checkVersion() const {
			return checkVersion(version);
		}

		void versionRequire() const {
			if(!checkVersion()) {
				std::cerr << "Build has unsupported encoding version " << version << "." << std::endl;
				throw(EncodingVersionError("Build has unsupported encoding version."));
			}
		}

		int32_t getVersion() const {
			return version;
		}

		KBuild* getBuild() {
			return build;
		}

		const KBuild* getBuild() const {
			return build;
		}

		void setBuild(KBuild* b) {
			build = b;
		}

		// Clears ownership without deleting anything.
		void softClear() {
			setBuild(NULL);
		}

		void clear() {
			if(build != NULL) {
				delete build;
			}
			softClear();
		}

		std::istream& load(std::istream& in, int verbosity);
		void loadFrom(const std::string& path, int verbosity);

		operator bool() const {
			return build != NULL;
		}

		KBuildFile() : version(MAX_VERSION), build(NULL) {}
		~KBuildFile() {
			clear();
		}
	};
}


#endif
