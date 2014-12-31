#include "kbuild.hpp"

using namespace std;

namespace Krane {
	// This is the scale applied by the scml compiler in the mod tools.
	const float_type KBuild::MODTOOLS_SCALE = 1.0/3;


	void KBuild::Symbol::Frame::getGeometry(Magick::Geometry& geo) const {
		using namespace Magick;

		const Image atlas = parent->parent->atlases[getAtlasIdx()].second;

		float_type w0 = atlas.columns();
		float_type h0 = atlas.rows();

		geo.xOff(size_t( floor(w0*atlas_bbox.x()) ));
		geo.yOff(size_t( floor(h0*atlas_bbox.y()) ));
		geo.width(size_t( floor(w0*atlas_bbox.w() + 0.5) ));
		geo.height(size_t( floor(h0*atlas_bbox.h() + 0.5) ));
	}

	Magick::Image KBuild::Symbol::Frame::getClipMask() const {
		using namespace Magick;
		using namespace std;

		Geometry geo;
		getGeometry(geo);
		geo.xOff(0);
		geo.yOff(0);

		// Size of the atlas.
		float_type w0, h0;
		{
			const Image atlas = parent->parent->atlases[getAtlasIdx()].second;

			w0 = atlas.columns();
			h0 = atlas.rows();
		}

		// Clip mask.
		Image mask(geo, ColorMono(true));
		mask.monochrome(true);
		mask.fillColor(ColorMono(false));

		list<Drawable> drawable_trigs;
		size_t ntrigs = uvwtriangles.size();
		list<Coordinate> coords;
		for(size_t i = 0; i < ntrigs; i++) {
			const uvwtriangle_type& trig = uvwtriangles[i];

			coords.push_back( Coordinate((trig.a[0] - atlas_bbox.x())*w0, (trig.a[1] - atlas_bbox.y())*h0) );
			coords.push_back( Coordinate((trig.b[0] - atlas_bbox.x())*w0, (trig.b[1] - atlas_bbox.y())*h0) );
			coords.push_back( Coordinate((trig.c[0] - atlas_bbox.x())*w0, (trig.c[1] - atlas_bbox.y())*h0) );

			drawable_trigs.push_back( DrawablePolygon(coords) );
			coords.clear();
		}
		MAGICK_WRAP(mask.draw(drawable_trigs));

		return mask;
	}

	Magick::Image KBuild::Symbol::Frame::getImage() const {
		using namespace Magick;
		using namespace std;

		// Bounding quad.
		Image quad;
		// Geometry of the bounding quad.
		Geometry geo;
		getGeometry(geo);
		// Size of the atlas.
		float_type w0, h0;
		{
			const Image atlas = parent->parent->atlases[getAtlasIdx()].second;

			w0 = atlas.columns();
			h0 = atlas.rows();

			quad = atlas;
			try {
				quad.crop(geo);
			}
			catch(Magick::Warning& w) {
				cerr << w.what() << endl;
				cerr << "When cropping " << int(w0) << "x" << int(h0) << " atlas " << parent->parent->atlases[getAtlasIdx()].first << " to " << geo.width() << "x" << geo.height() << "+" << geo.xOff() << "+" << geo.yOff() << " to select image " << getUnixPath() << endl;
			}
		}


		// Clip mask.
		Image mask = getClipMask();


		// Returned image (clipped quad).
		Image img = Image(geo, "transparent");
		img.clipMask(mask);
		MAGICK_WRAP(img.composite( quad, Geometry(0, 0), OverCompositeOp ));
		img.clipMask(Image());


		// This is to reverse the scaling down applied by the mod tools' scml compiler.
		Geometry scaling_geo;
		scaling_geo.aspect(true);
		scaling_geo.width(bbox.int_w());
		scaling_geo.height(bbox.int_h());
		try {
			img.resize(scaling_geo);
		}
		catch(Magick::Warning& w) {
			cerr << w.what() << endl;
			cerr << "When reszing " << img.columns() << "x" << img.rows() << " image " << getUnixPath() << " to " << scaling_geo.width() << "x" << scaling_geo.height() << endl;
		}

		img.flip();

		return img;
	}

	Magick::Image KBuild::getClipMask(size_t idx) const {
		using namespace Magick;
		using namespace std;

		typedef symbolmap_t::const_iterator symbol_iter;
		typedef Symbol::framelist_t::const_iterator frame_iter;

		Geometry atlasgeo;
		{
			atlaslist_t::const_iterator atit = atlases.begin();
			std::advance(atit, idx);
			atlasgeo = atit->second.size();
		}

		// Clip mask.
		Image mask(atlasgeo, ColorMono(true));
		mask.monochrome(true);

		for(symbol_iter symit = symbols.begin(); symit != symbols.end(); ++symit) {
			for(frame_iter frit = symit->second.frames.begin(); frit != symit->second.frames.end(); ++frit) {
				if(size_t(frit->getAtlasIdx()) == idx) {
					Geometry framegeo;
					frit->getGeometry(framegeo);

					Image framemask = frit->getClipMask();

					MAGICK_WRAP(mask.composite( framemask, framegeo, OverCompositeOp ));
				}
			}
		}

		return mask;
	}

	Magick::Image KBuild::getMarkedAtlas(size_t idx, Magick::Color c) const {
		using namespace Magick;
		using namespace std;

		atlas_t atlas;
		{
			atlaslist_t::const_iterator atit = atlases.begin();
			std::advance(atit, idx);
			atlas = atit->second;
		}

		// Clip mask.
		Image mask = getClipMask(idx);

		// Inverse clip mask;
		Image inversemask = mask;
		inversemask.negate();

		// Background.
		Magick::Image bg(atlas.size(), c);

		// Returned image.
		Magick::Image markedatlas(atlas.size(), "transparent");

		markedatlas.clipMask(inversemask);
		MAGICK_WRAP(markedatlas.composite( bg, Geometry(0, 0), OverCompositeOp ));

		markedatlas.clipMask(mask);
		MAGICK_WRAP(markedatlas.composite( atlas, Geometry(0, 0), OverCompositeOp ));

		markedatlas.clipMask(Image());

		markedatlas.flip();
		return markedatlas;
	}
}
