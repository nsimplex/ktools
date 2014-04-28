#include "kbuild.hpp"

using namespace std;

namespace Krane {
	// This is the scale applied by the scml compiler in the mod tools.
	const KBuild::float_type KBuild::MODTOOLS_SCALE = 1.0/3;


	Magick::Image KBuild::Symbol::Frame::getImage() const {
		using namespace Magick;
		using namespace std;

		/*
		cout << "Image of " << getUnixPath() << ":" << endl;
		cout << atlas_bbox.w() << "x" << atlas_bbox.h() << "+" << atlas_bbox.x() << "+" << atlas_bbox.y() << endl;
		cout << ": " << bbox.w() << "x" << bbox.h() << "+" << bbox.x() << "+" << bbox.y() << endl;
		cout << "Atlas idx: " << getAtlasIdx() << endl;
		*/

		// Bounding quad.
		Image quad;
		// Size of the atlas.
		float_type w0, h0;
		// Offset used in the cropping.
		bbox_type::vector_type offset;
		{
			Image atlas = parent->parent->atlases[getAtlasIdx()].second;

			w0 = atlas.columns();
			h0 = atlas.rows();

			offset[0] = w0*atlas_bbox.x();
			offset[1] = h0*atlas_bbox.y();

			Geometry cropgeo;
			cropgeo.xOff(size_t( floor(offset[0]) ));
			cropgeo.yOff(size_t( floor(offset[1]) ));
			cropgeo.width(size_t( floor(w0*atlas_bbox.w()) ));
			cropgeo.height(size_t( floor(h0*atlas_bbox.h()) ));

			quad = atlas;
			try {
				quad.crop(cropgeo);
			}
			catch(Magick::Warning& w) {
				cerr << w.what() << endl;
				cerr << "When cropping " << int(w0) << "x" << int(h0) << " atlas " << parent->parent->atlases[getAtlasIdx()].first << " to " << cropgeo.width() << "x" << cropgeo.height() << "+" << cropgeo.xOff() << "+" << cropgeo.yOff() << " to select image " << getUnixPath() << endl;
			}
		}


		Geometry geo = quad.size();

		// Clip mask.
		Image mask;
		mask.monochrome(true);
		mask.backgroundColor(ColorMono(true));
		mask.fillColor(ColorMono(false));
		mask.size(geo);

		list<Drawable> drawable_trigs;
		size_t ntrigs = uvwtriangles.size();
		list<Coordinate> coords;
		for(size_t i = 0; i < ntrigs; i++) {
			const uvwtriangle_type& trig = uvwtriangles[i];

			coords.push_back( Coordinate(trig.a[0]*w0 - offset[0], trig.a[1]*h0 - offset[1]) );
			coords.push_back( Coordinate(trig.b[0]*w0 - offset[0], trig.b[1]*h0 - offset[1]) );
			coords.push_back( Coordinate(trig.c[0]*w0 - offset[0], trig.c[1]*h0 - offset[1]) );

			drawable_trigs.push_back( DrawablePolygon(coords) );
			coords.clear();
		}
		MAGICK_WRAP(mask.draw(drawable_trigs));


		// Returned image (clipped quad).
		Image img = Image(geo, "transparent");
		img.clipMask(mask);
		MAGICK_WRAP(img.composite( quad, Geometry(0, 0), OverCompositeOp ));


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
}
