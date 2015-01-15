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


#ifndef KTOOLS_ATLAS_HPP
#define KTOOLS_ATLAS_HPP

#include "ktools_common.hpp"
#include "file_abstraction.hpp"
#include "algebra.hpp"
#include "ktex/ktex.hpp"
#include "image_operations.hpp"

#include <pugixml/pugixml.hpp>

// To avoid sampling errors or something.
#define ATLAS_PIXEL_THRESHOLD 0.5

namespace KTools {
	class Atlas;

	/*
	 * The mutable members are for lazy evaluation.
	 */
	class AtlasSheet {
		friend class Atlas;

	public:
		static const size_t MAX_WIDTH = 2048;
		static const size_t MAX_HEIGHT = 2048;

		typedef std::deque< std::pair<Compat::UnixPath, Magick::Image> > imagelist_t;

		typedef imagelist_t::value_type value_type;
		typedef imagelist_t::iterator iterator;
		typedef imagelist_t::const_iterator const_iterator;

		// In pixels.
		typedef BoundingBox<size_t> bbox_t;
		typedef std::deque<bbox_t> bbox_list_t;

		typedef BoundingBox<float_type> uv_bbox_t;

	private:
		const Atlas* _parent;

		mutable Maybe<Compat::UnixPath> subpath;

		mutable bool pending_synthesis;
		mutable bool pending_analysis;

		mutable imagelist_t images;

		mutable Magick::Image final_image;
		mutable bbox_list_t bboxes;

		mutable size_t width, height;

		struct geometrical_state {
			size_t x, y;
			size_t current_row_height;
		};
		
		mutable geometrical_state geo_state;

		void update_geometrical_state() const {
			const bbox_t* final_bbox = NULL;
			size_t max_ymax = 0;
			for(bbox_list_t::const_iterator bbox_it = bboxes.begin(); bbox_it != bboxes.end(); ++bbox_it) {
				if(final_bbox == NULL || final_bbox->lexYXLess(*bbox_it)) {
					final_bbox = &*bbox_it;
				}
				max_ymax = std::max(max_ymax, bbox_it->ymax());
			}
			if(final_bbox != NULL) {
				geo_state.x = final_bbox->x() + final_bbox->w();
				geo_state.y = final_bbox->y();
				geo_state.current_row_height = max_ymax - geo_state.y;
			}
		}

		static inline float_type reflect(float_type v) {
			return 1 - v;
		}

		inline uv_bbox_t mapUV(const bbox_t& bbox) const {
			const float_type u_margin = ATLAS_PIXEL_THRESHOLD/width;
			const float_type v_margin = ATLAS_PIXEL_THRESHOLD/height;

			const float_type u_factor = float_type(1)/width;
			const float_type v_factor = float_type(1)/height;

			uv_bbox_t uv_bbox;

			uv_bbox.x(bbox.x()*u_factor + u_margin);
			uv_bbox.y(reflect(bbox.ymax()*v_factor - v_margin));

			uv_bbox.xmax(bbox.xmax()*u_factor - u_margin);
			uv_bbox.ymax(reflect(bbox.y()*v_factor + v_margin));

			return uv_bbox;
		}

		inline bbox_t unmapUV(const uv_bbox_t& uv_bbox) const {
			const float_type x_factor = float_type(width);
			const float_type y_factor = float_type(height);

			const float_type x = uv_bbox.x()*x_factor;
			const float_type y = reflect(uv_bbox.ymax())*y_factor;

			const float_type xmax = uv_bbox.xmax()*x_factor;
			const float_type ymax = reflect(uv_bbox.y())*y_factor;

			bbox_t bbox;

			bbox.x( size_t(floor(x)) );
			bbox.y( size_t(floor(y)) );

			bbox.xmax( size_t(ceil(xmax)) );
			bbox.ymax( size_t(ceil(ymax)) );

			return bbox;
		}

		void load(pugi::xml_node node, const VirtualPath& basedir, int verbosity);
		void dump(pugi::xml_node node, const VirtualPath& basedir, int verbosity) const;

		void saveImages(const VirtualPath& output_dir, bool clear_on_done, int verbosity) const;

		void analyze_image(imagelist_t::iterator img_it,bbox_list_t::const_iterator bbox_it) const;

	public:
		AtlasSheet(const Atlas* _p = NULL) : _parent(_p) {
			clear();
		}

		AtlasSheet(const AtlasSheet& s) {
			copyFrom(s);
		}

		void synthesize() const;
		void analyze() const;

		void copyFrom(const AtlasSheet& s) {
			_parent = s._parent;
			subpath = s.subpath;
			pending_synthesis = s.pending_synthesis;
			pending_analysis = s.pending_analysis;
			images = s.images;
			final_image = s.final_image;
			bboxes = s.bboxes;
			width = s.width;
			height = s.height;
			geo_state = s.geo_state;
		}

		// all but parent and subpath.
		void clear() {
			pending_synthesis = true;
			pending_analysis = false;

			images.clear();
			final_image = Magick::Image();
			bboxes.clear();

			width = 0;
			height = 0;

			geo_state = geometrical_state();
		}

		const Atlas& parent() const {
			if(_parent == NULL) {
				throw KToolsError("AtlasSheet has NULL parent.");
			}
			return *_parent;
		}

		AtlasSheet& operator=(const AtlasSheet& s) {
			copyFrom(s);
			return *this;
		}

		bool hasSubPath() const {
			return subpath != nil;
		}

		const Compat::UnixPath& getSubPath() const {
			return subpath.ref();
		}

		void setSubPath(const Compat::UnixPath& p) const {
			subpath = Just(p);
		}

		iterator begin() {
			return images.begin();
		}

		const_iterator begin() const {
			return images.begin();
		}

		iterator end() {
			return images.end();
		}

		const_iterator end() const {
			return images.end();
		}

		bool addImage(const Compat::UnixPath& id, Magick::Image img) {
			analyze();

			const size_t w = img.columns();
			const size_t h = img.rows();

			if(geo_state.x + w > MAX_WIDTH) {
				const size_t new_y = geo_state.y + geo_state.current_row_height;

				if(new_y + h > MAX_HEIGHT) {
					return false;
				}

				geo_state.y = new_y;
				geo_state.x = 0;
				geo_state.current_row_height = h;

				height = new_y + h;
			}
			else {
				const size_t old_height = geo_state.current_row_height;
				geo_state.current_row_height = std::max(old_height, h);

				const size_t delta = geo_state.current_row_height - old_height;

				height += delta;
			}

			bboxes.push_back( bbox_t() );
			bbox_t& bbox = bboxes.back();

			bbox.setDimensions( geo_state.x, geo_state.y, w, h );

			geo_state.x += w;
			width = std::max( width, geo_state.x );

			images.push_back( value_type() );
			
			value_type& v = images.back();

			v.first = id;
			v.second = img;

			pending_synthesis = true;

			return true;
		}

		Magick::Image findImage(const Compat::UnixPath& id) const {
			analyze();
			for(const_iterator it = begin(); it != end(); ++it) {
				if(it->first == id) {
					return it->second;
				}
			}
			return Magick::Image();
		}

		Magick::Image getFinalImage() const {
			synthesize();
			return final_image;
		}
	};

	class Atlas {
	public:
		typedef AtlasSheet Sheet;

		typedef AtlasSheet sheet_t;
		typedef std::vector<sheet_t> sheetlist_t;

		typedef sheetlist_t::iterator sheet_iterator;
		typedef sheetlist_t::const_iterator sheet_const_iterator;

		typedef ImOp::operation_t<const KTEX::File&, Magick::Image> decompressor_t;
		typedef ImOp::binary_operation_t<KTEX::File&, Magick::Image> compressor_t;

		typedef ImOp::operation_ref_t<decompressor_t> decompressor_ref_t;
		typedef ImOp::operation_ref_t<compressor_t> compressor_ref_t;

	private:
		mutable DataFormatter fmt;

		std::string name;
		VirtualPath path;
		VirtualPath texture_path;

		bool is_default_texture_path;

		decompressor_ref_t decompressor;
		compressor_ref_t compressor;

		sheetlist_t sheets;

		mutable bool dirty_sheetpaths;

		void cleanSheetPaths() const {
			if(dirty_sheetpaths) {
				const size_t nsheets = sheets.size();
				unsigned int i = 0;
				for(sheet_const_iterator sheet_it = sheets.begin(); sheet_it != sheets.end(); ++sheet_it) {
					if(!sheet_it->hasSubPath()) {
						if(nsheets == 1) {
							sheet_it->setSubPath(getTexturePath());
						}
						else {
							sheet_it->setSubPath( getTexturePath().removeExtension() + fmt("-%u.tex", i++) );
						}
					}
				}
				dirty_sheetpaths = false;
			}
		}


		sheet_t& pushSheet() {
			sheets.push_back( AtlasSheet(this) );
			return sheets.back();
		}
	

		std::istream& do_load(std::istream& in, int verbosity);
		std::ostream& do_dump(std::ostream& out, int verbosity) const;

	public:
		Atlas() : is_default_texture_path(true), dirty_sheetpaths(true) {
			setPath("atlas.xml");
			sheets.reserve(1);
		}

		const std::string& getName() const {
			return name;
		}

		void setName(const std::string& s) {
			name = s;
			path = path.dirname()/name;
			path += ".xml";
		}

		const VirtualPath& getPath() const {
			return path;
		}

		void setPath(const VirtualPath& p) {
			path = p;
			name = path.basename().removeExtension();
			if(is_default_texture_path) {
				texture_path = name + ".tex";
			}
		}

		const VirtualPath& getTexturePath() const {
			return texture_path;
		}

		void setTexturePath(const VirtualPath& p) {
			texture_path = p;
			is_default_texture_path = false;
		}

		const decompressor_t& getDecompressor() const {
			return *decompressor;
		}

		void setDecompressor(decompressor_ref_t ref) {
			decompressor = ref;
		}

		const compressor_t& getCompressor() const {
			return *compressor;
		}

		void setCompressor(compressor_ref_t ref) {
			compressor = ref;
		}

		void addImage(const Compat::UnixPath& id, Magick::Image img) {
			for(sheet_iterator sheet_it = sheets.begin(); sheet_it != sheets.end(); ++sheet_it) {
				if(sheet_it->addImage(id, img)) {
					return;
				}
			}
			if(!pushSheet().addImage(id, img)) {
				throw KToolsError( fmt("An atlas texture file has a maximum size of %ux%u. Impossible to fit image '%s' of size %ux%u.",
							(unsigned)Sheet::MAX_WIDTH,
							(unsigned)Sheet::MAX_HEIGHT,
							id.c_str(),
							(unsigned)img.columns(),
							(unsigned)img.rows()) );
			}
		}

		void synthesize() const {
			for(sheet_const_iterator sheet_it = sheets.begin(); sheet_it != sheets.end(); ++sheet_it) {
				sheet_it->synthesize();
			}
		}
		void analyze() const {
			for(sheet_const_iterator sheet_it = sheets.begin(); sheet_it != sheets.end(); ++sheet_it) {
				sheet_it->analyze();
			}
		}

		sheet_const_iterator begin() const {
			return sheets.begin();
		}

		sheet_const_iterator end() const {
			return sheets.end();
		}
		
		std::istream& load(std::istream& in, const VirtualPath& p, int verbosity = -1) {
			setPath(p);
			return do_load(in, verbosity);
		}

		void load(const VirtualPath& p, int verbosity = -1) {
			std::istream* in = p.open_in();
			load(*in, p, verbosity);
			delete in;
		}

		std::ostream& dump(std::ostream& out, const VirtualPath& p, int verbosity = -1) {
			setPath(p);
			return do_dump(out, verbosity);
		}

		void dump(const VirtualPath& p, int verbosity = -1) {
			std::ostream* out = p.open_out();
			dump(*out, p, verbosity);
			delete out;
		}

		void saveImages(const VirtualPath& output_dir, bool clear_on_done, int verbosity = -1) const {
			for(sheet_const_iterator sheet_it = begin(); sheet_it != end(); ++sheet_it) {
				sheet_it->saveImages(output_dir, clear_on_done, verbosity);
			}
		}
	};

	class AtlasCache {
	public:
		class AtlasRef;
		
	private:
		friend class AtlasRef;

		typedef std::map< VirtualPath, Atlas* > cache_t;
		static cache_t cache;

		typedef std::multiset< Atlas* > refcount_t;
		static refcount_t refcount;

	public:
		class AtlasRef {
			friend class AtlasCache;

			Atlas* ptr;

			AtlasRef(Atlas* a) : ptr(a) {
				assert( a != NULL );
				AtlasCache::refcount.insert(a);
			}

		public:
			AtlasRef(const AtlasRef& aref) : ptr(aref.ptr) {
				AtlasCache::refcount.insert(ptr);
			}

			virtual ~AtlasRef() {
				AtlasCache::refcount.erase(ptr);
				if(AtlasCache::refcount.count(ptr) == 0) {
					AtlasCache::cache.erase(ptr->getPath());
					delete ptr;
				}
			}

			Atlas& operator*() {
				return *ptr;
			}

			Atlas* operator->() {
				return ptr;
			}
		};

		static AtlasRef open(const VirtualPath& p) {
			Atlas* a = NULL;

			cache_t::iterator match = cache.find(p);
			if(match == cache.end()) {
				a = new Atlas();
				a->load(p);
				cache[p] = a;
			}
			else {
				a = match->second;
			}

			return AtlasRef(a);
		}
	
	private:
		AtlasCache() {}
		AtlasCache(const AtlasCache&) {}
	};

	typedef AtlasCache::AtlasRef AtlasRef;
}

#endif
