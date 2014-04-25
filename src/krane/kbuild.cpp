#include "kbuild.hpp"

using namespace std;

namespace Krane {
	// This is the scale applied by the scml compiler in the mod tools.
	const KBuild::float_type KBuild::MODTOOLS_SCALE = 1.0/3;


	Magick::Image KBuild::Symbol::Frame::getImage() const {
		using namespace Magick;
		using namespace std;

		// Bounding quad.
		Image quad;
		// Size of the atlas.
		float_type w0, h0;
		// Offset used in the cropping.
		bbox_type::vector_type offset;
		{
			Image atlas = parent->parent->getFrontAtlas();

			w0 = atlas.columns();
			h0 = atlas.rows();

			offset[0] = w0*atlas_bbox.x();
			offset[1] = h0*(1 - atlas_bbox.ymax());

			Geometry cropgeo;
			cropgeo.xOff(size_t( floor(offset[0]) ));
			cropgeo.yOff(size_t( floor(offset[1]) ));
			cropgeo.width(size_t( floor(w0*atlas_bbox.w()) ));
			cropgeo.height(size_t( floor(h0*atlas_bbox.h()) ));

			quad = atlas;
			quad.crop(cropgeo);
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
			const triangle_type& trig = uvwtriangles[i];

			coords.push_back( Coordinate(trig.a[0]*w0 - offset[0], (1 - trig.a[1])*h0 - offset[1]) );
			coords.push_back( Coordinate(trig.b[0]*w0 - offset[0], (1 - trig.b[1])*h0 - offset[1]) );
			coords.push_back( Coordinate(trig.c[0]*w0 - offset[0], (1 - trig.c[1])*h0 - offset[1]) );

			drawable_trigs.push_back( DrawablePolygon(coords) );
			coords.clear();
		}
		mask.draw(drawable_trigs);


		// Returned image (clipped quad).
		Image img = Image(geo, "transparent");
		img.clipMask(mask);
		img.composite( quad, Geometry(0, 0), OverCompositeOp );


		// This is to reverse the scaling down applied by the mod tools' scml compiler.
		Geometry scaling_geo;
		scaling_geo.aspect(true);
		scaling_geo.width(bbox.w());
		scaling_geo.height(bbox.h());
		img.resize(scaling_geo);

		return img;
	}
}
