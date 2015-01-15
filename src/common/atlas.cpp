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

#include "atlas.hpp"
#include "ktools_bit_op.hpp"
#include "binary_io_utils.hpp"

#include "ktex/ktex.hpp"

#include <pugixml/pugixml.hpp>

#include <cctype>

using namespace pugi;

using std::string;
using std::cout;
using std::cerr;
using std::endl;

namespace {
	static bool iequals(const std::string &a, const std::string& b) {
		const size_t a_l = a.length();
		if(a_l != b.length()) {
			return false;
		}
		for(size_t i = 0; i < a_l; i++) {
			if(tolower(a[i]) != tolower(b[i])) {
				return false;
			}
		}
		return true;
	}

	template<int (*f)(int)>
	static std::string& mapstr(std::string& s) {
		const size_t l = s.length();
		for(size_t i = 0; i < l; i++) {
			s[i] = (char)(*f)(s[i]);
		}
		return s;
	}

	xml_attribute require_attr(xml_node node, const char *name) {
		xml_attribute ret = node.attribute(name);
		if(!ret) {
			throw KTools::KToolsError(std::string("required attribute '") + name + "' missing.");
		}
		return ret;
	}
}

namespace KTools {
	AtlasCache::cache_t AtlasCache::cache;
	AtlasCache::refcount_t AtlasCache::refcount;

	//

	xml_node ichild(xml_node node, std::string tag) {
		xml_node child = node.child(tag.c_str());
		if(child) return child;

		child = node.child(mapstr<tolower>(tag).c_str());
		if(child) return child;

		child = node.child(mapstr<toupper>(tag).c_str());
		return child;
	}

	xml_node isibling(xml_node node, std::string tag) {
		xml_node sibling = node.next_sibling(tag.c_str());
		if(sibling) return sibling;

		sibling = node.next_sibling(mapstr<tolower>(tag).c_str());
		if(sibling) return sibling;

		sibling = node.next_sibling(mapstr<toupper>(tag).c_str());
		return sibling;
	}

	//

	void AtlasSheet::synthesize() const {
		if(!pending_synthesis) return;

		width = BitOp::Pow2Rounder::roundUp(width);
		height = BitOp::Pow2Rounder::roundUp(height);

		final_image = Magick::Image( Magick::Geometry(width, height), "transparent" );

		bbox_list_t::const_iterator bbox_it = bboxes.begin();
		for(const_iterator it = images.begin(); it != images.end(); ++it, ++bbox_it) {
			final_image.composite(it->second, bbox_it->x(), bbox_it->y(), Magick::OverCompositeOp);
		}

		pending_synthesis = false;
	}

	void AtlasSheet::analyze_image(AtlasSheet::imagelist_t::iterator img_it, bbox_list_t::const_iterator bbox_it) const {
		img_it->second = final_image;
		Magick::Geometry geo( bbox_it->w(), bbox_it->h(), bbox_it->x(), bbox_it->y() );
		//cerr << "+" << bbox_it->x() << "+" << bbox_it->y() << " " << bbox_it->w() << "x" << bbox_it->h() << endl;
		img_it->second.crop(geo);
		img_it->second.page(geo);
	}

	void AtlasSheet::analyze() const {
		if(!pending_analysis) return;

		if(final_image.columns() == 0 || final_image.rows() == 0) return;

		iterator it = images.begin();
		for(bbox_list_t::const_iterator bbox_it = bboxes.begin(); bbox_it != bboxes.end(); ++bbox_it, ++it) {
			analyze_image(it, bbox_it);
		}

		update_geometrical_state();

		pending_analysis = false;
	}

	//

	void AtlasSheet::load(xml_node node, const VirtualPath& basedir, int verbosity) {
		Compat::UnixPath texture_filename = require_attr(node, "filename").as_string();
		setSubPath(texture_filename);

		try {
			if(verbosity >= 1) {
				std::cout << "Loading atlas texture file data for '" << texture_filename << "'" << std::endl;
			}

			{
				KTEX::File tex;

				VirtualPath tex_path = basedir/std::string(texture_filename);
				std::istream* in = tex_path.open_in(std::ifstream::binary);
				tex.load( *in, std::min(0, verbosity) );
				delete in;

				final_image = parent().getDecompressor()(tex);
			}

			if(final_image.columns() == 0 || final_image.rows() == 0) {
				throw KToolsError("atlas texture file has zero size.");
			}

			width = final_image.columns();
			height = final_image.rows();
			
			xml_node elements_node = node.next_sibling();
			if(!elements_node || !iequals(elements_node.name(), "Elements")) {
				throw KToolsError("Elements tag expected for next sibling.");
			}

			for(xml_node el = ichild(elements_node, "Element"); el; el = isibling(el, "Element")) {
				Compat::UnixPath id = require_attr(el, "name").as_string();

				uv_bbox_t uv_bbox;
				uv_bbox.x(require_attr(el, "u1").as_double());
				uv_bbox.xmax(require_attr(el, "u2").as_double());
				uv_bbox.y(require_attr(el, "v1").as_double());
				uv_bbox.ymax(require_attr(el, "v2").as_double());

				bboxes.push_back( unmapUV(uv_bbox) );

				images.resize(images.size() + 1);
				images.back().first = require_attr(el, "name").as_string();

				if(verbosity >= 2) {
					cout << "Got element '" << id << "'." << endl;
				}
			}

			pending_synthesis = false;
			pending_analysis = true;
			//analyze();
		}
		catch(const KToolsError& err) {
			throw KToolsError(std::string("under texture file '") + getSubPath() + "': " + err.what());
		}
	}

	void AtlasSheet::dump(xml_node node, const VirtualPath& basedir, int verbosity) const {
		const std::string& texture_filename = getSubPath();

		try {
			if(verbosity >= 1) {
				std::cout << "Saving atlas texture file data for '" << texture_filename << "'" << std::endl;
			}

			synthesize();

			node.append_attribute("filename") = texture_filename.c_str();

			xml_node elements_node = node.parent().append_child("Elements");
			{
				const_iterator img_it;
				bbox_list_t::const_iterator bbox_it;

				for(img_it = images.begin(), bbox_it = bboxes.begin(); img_it != images.end(); ++img_it, ++bbox_it) {
					xml_node el = elements_node.append_child("Element");

					el.append_attribute("name") = img_it->first.c_str();

					uv_bbox_t uv_bbox = mapUV(*bbox_it);

					el.append_attribute("u1") = uv_bbox.x();
					el.append_attribute("u2") = uv_bbox.xmax();
					el.append_attribute("v1") = uv_bbox.y();
					el.append_attribute("v2") = uv_bbox.ymax();
				}
			}

			{
				KTEX::File tex;
				
				parent().getCompressor()(tex, final_image);

				VirtualPath tex_path = basedir/texture_filename;
				std::ostream* out = tex_path.open_out(std::ofstream::binary);
				tex.dump( *out, std::min(0, verbosity) );
				delete out;
			}
		}
		catch(const KToolsError& err) {
			throw KToolsError(std::string("under texture file '") + getSubPath() + "': " + err.what());
		}
	}

	//

	void AtlasSheet::saveImages(const VirtualPath& output_dir, bool clear_on_done, int verbosity) const {
		iterator it = images.begin();
		for(bbox_list_t::const_iterator bbox_it = bboxes.begin(); bbox_it != bboxes.end(); ++bbox_it, ++it) {
			if(it->second.columns() == 0) {
				analyze_image(it, bbox_it);
			}

			Compat::Path output_path = (output_dir/it->first).replaceExtension("png", true);
			if(verbosity >= 3) {
				cout << "Writing '" << output_path << "'..." << endl;
			}
			MAGICK_WRAP( it->second.write(output_path) );

			if(clear_on_done) {
				it->second = Magick::Image();
			}
		}
	}

	//

	std::istream& Atlas::do_load(std::istream& in, int verbosity) {
		BinIOHelper::sanitizeStream(in);

		try {
			if(verbosity >= 0) {
				std::cout << "Loading atlas from '" << getPath() << "'..." << std::endl;
			}

			xml_document atlas_file_doc;

			VirtualPath basedir = getPath().dirname();

			xml_parse_result result = atlas_file_doc.load(in, parse_default, encoding_utf8);
			if(result.status != status_ok) {
				throw KToolsError(result.description());
			}

			xml_node atlas_node = ichild(atlas_file_doc, "Atlas");

			if(!atlas_node) {
				throw KToolsError("Atlas tag expected as document child.");
			}
			if(isibling(atlas_node, "Atlas")) {
				throw KToolsError("single Atlas tag expected.");
			}

			bool found_any_texture = false;
			for(xml_node child = ichild(atlas_node, "Texture"); child; child = isibling(child, "Texture")) {
				sheet_t& sheet = pushSheet();
				sheet.load(child, basedir, verbosity);
				found_any_texture = true;
			}

			if(!found_any_texture) {
				std::cerr << "WARNING: the atlas '" << getPath() << "' does not specify any texture files. No output generated." << std::endl;
			}

			if(verbosity >= 0) {
				std::cout << "Loaded atlas from '" << getPath() << "'." << std::endl;
			}
		}
		catch(const KToolsError& err) {
			throw KToolsError(std::string("Failed to load xml document '") + getPath() + "': " + err.what() );
		}

		return in;
	}

	std::ostream& Atlas::do_dump(std::ostream& out, int verbosity) const {
		BinIOHelper::sanitizeStream(out);
		cleanSheetPaths();

		try {
			if(verbosity >= 0) {
				std::cout << "Saving atlas to '" << getPath() << "'..." << std::endl;
			}

			xml_document atlas_file_doc;

			VirtualPath basedir = getPath().dirname();

			xml_node atlas_node = atlas_file_doc.append_child("Atlas");

			for(sheet_const_iterator it = sheets.begin(); it != sheets.end(); ++it) {
				xml_node texture_node = atlas_node.append_child("Texture");
				it->dump(texture_node, basedir, verbosity);
			}

			atlas_file_doc.save(out, "\t", format_default, encoding_utf8);

			if(verbosity >= 0) {
				std::cout << "Saved atlas to '" << getPath() << "'." << std::endl;
			}
		}
		catch(const KToolsError& err) {
			throw KToolsError(std::string("Failed to save xml document '") + getPath() + "': " + err.what());
		}

		return out;
	}
}
