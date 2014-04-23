#include "scml.hpp"
#include <pugixml/pugixml.hpp>

#include <limits>

using namespace std;
using namespace Krane;
using namespace pugi;


/***********************************************************************/

/*
 * Global linear scale (applied to dimensions, translations, etc.).
 *
 * This is NOT arbitrary, being needed for translations and positions to
 * match the expected behaviour. Hence it is not configurable.
 */
static const computations_float_type SCALE = 1.0/3;


/***********************************************************************/


static void sanitize_stream(std::ostream& out) {
	out.imbue(locale::classic());
}

static inline int tomilli(float x) {
	return int(ceilf(1000.0f*x));
}

static inline int tomilli(double x) {
	return int(ceil(1000.0*x));
}


/***********************************************************************/


namespace {
	struct BuildSymbolFrameMetadata {
		computations_float_type pivot_x;
		computations_float_type pivot_y;
	};

	struct BuildSymbolMetadata : public std::vector<BuildSymbolFrameMetadata> {
		uint32_t folder_id;
	};

	// The map keys are the build symbol names' hashes.
	typedef map<hash_t, BuildSymbolMetadata> BuildMetadata;


	/***********/


	struct GenericState {
	//	KTools::DataFormatter fmt;
	};

	struct BuildExporterState : public GenericState {
		uint32_t folder_id;

		BuildExporterState() : folder_id(0) {}
	};

	struct BuildSymbolExporterState : public GenericState {
		uint32_t file_id;

		BuildSymbolExporterState() : file_id(0) {}
	};

	struct AnimationBankCollectionExporterState : public GenericState {
		uint32_t entity_id;

		AnimationBankCollectionExporterState() : entity_id(0) {}
	};

	struct AnimationBankExporterState : public GenericState {
		uint32_t animation_id;

		AnimationBankExporterState() : animation_id(0) {}
	};

	struct AnimationExporterState : public GenericState {
		uint32_t key_id;
		computations_float_type running_length;

		AnimationExporterState() : key_id(0), running_length(0) {}
	};

	struct AnimationFrameExporterState : public GenericState {
		uint32_t timeline_id;
		uint32_t z_index;

		computations_float_type last_sort_order;

		AnimationFrameExporterState() : timeline_id(0), z_index(0) {
			last_sort_order = numeric_limits<computations_float_type>::infinity();
		}
	};
}


/***********************************************************************/


static void exportBuild(xml_node& spriter_data, BuildMetadata& bmeta, const KBuild& bild);

static void exportBuildSymbol(xml_node& spriter_data, BuildExporterState& s, BuildSymbolMetadata& symmeta, const KBuild::Symbol& sym);

static void exportBuildSymbolFrame(xml_node& folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame);


static void exportAnimationBankCollection(xml_node& spriter_data, const BuildMetadata& bmeta, const KAnimBankCollection& C);

static void exportAnimationBank(xml_node& spriter_data, AnimationBankCollectionExporterState& s, const BuildMetadata& bmeta, const KAnimBank& B);

static void exportAnimation(xml_node& entity, AnimationBankExporterState& s, const BuildMetadata& bmeta, const KAnim& A);

static void exportAnimationFrame(xml_node& mainline, xml_node& animation, AnimationExporterState& s, const BuildMetadata& bmeta, const KAnim::Frame& frame);

static void exportAnimationFrameElement(xml_node& mainline_key, xml_node& animation, AnimationFrameExporterState& s, const BuildSymbolMetadata& symmeta, const KAnim::Frame::Element& elem);


/***********************************************************************/


void Krane::exportToSCML(std::ostream& out, const KBuild& bild, const KAnimBankCollection& banks) {
	sanitize_stream(out);

	xml_document scml;
	xml_node decl = scml.prepend_child(node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";

	xml_node spriter_data = scml.append_child("spriter_data");

	spriter_data.append_attribute("scml_version") = "1.0";
	spriter_data.append_attribute("generator") = "BrashMonkey Spriter";
	spriter_data.append_attribute("generator_version") = "b5";

	BuildMetadata bmeta;
	exportBuild(spriter_data, bmeta, bild);

	exportAnimationBankCollection(spriter_data, bmeta, banks);

	scml.save(out, "\t", format_default, encoding_utf8);
}

/***********************************************************************/

static void exportBuild(xml_node& spriter_data, BuildMetadata& bmeta, const KBuild& bild) {
	typedef KBuild::symbolmap_const_iterator symbolmap_iterator;

	BuildExporterState local_s;

	for(symbolmap_iterator it = bild.symbols.begin(); it != bild.symbols.end(); ++it) {
		hash_t h = it->first;
		const KBuild::Symbol& sym = it->second;
		exportBuildSymbol(spriter_data, local_s, bmeta[h], sym);
	}
}

static void exportBuildSymbol(xml_node& spriter_data, BuildExporterState& s, BuildSymbolMetadata& symmeta, const KBuild::Symbol& sym) {
	typedef KBuild::frame_const_iterator frame_iterator;

	const uint32_t folder_id = s.folder_id++;

	// DEBUG
	cout << "Exporting symbol " << sym.getName() << endl;
	// END DEBUG

	xml_node folder = spriter_data.append_child("folder");

	folder.append_attribute("id") = folder_id;
	folder.append_attribute("name") = sym.getUnixPath().c_str();

	BuildSymbolExporterState local_s;

	symmeta.resize(sym.frames.size());
	for(frame_iterator it = sym.frames.begin(); it != sym.frames.end(); ++it) {
		const KBuild::Symbol::Frame& frame = *it;
		exportBuildSymbolFrame(folder, local_s, symmeta[local_s.file_id], frame);
	}

	symmeta.folder_id = folder_id;
}

static void exportBuildSymbolFrame(xml_node& folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame) {
	typedef KBuild::float_type float_type;

	const uint32_t file_id = s.file_id++;

	// DEBUG
	cout << "\tExporting frame #" << file_id << endl;
	cout << "\t\tframenum: " << frame.framenum << endl;
	cout << "\t\tduration: " << frame.duration << endl;
	// END DEBUG

	xml_node file = folder.append_child("file");

	float_type x, y;
	int w, h;
	x = frame.bbox.x();
	y = frame.bbox.y();
	w = frame.bbox.int_w();
	h = frame.bbox.int_h();

	computations_float_type pivot_x = 0.5 - (x/w);//*SCALE;
	computations_float_type pivot_y = 0.5 + (y/h);//*SCALE;

	framemeta.pivot_x = pivot_x;
	framemeta.pivot_y = pivot_y;

	file.append_attribute("id") = file_id;
	file.append_attribute("name") = frame.getUnixPath().c_str();
	file.append_attribute("width") = w;
	file.append_attribute("height") = h;
	file.append_attribute("pivot_x") = pivot_x;
	file.append_attribute("pivot_y") = pivot_y;
}

/***********************************************************************/

static void exportAnimationBankCollection(xml_node& spriter_data, const BuildMetadata& bmeta, const KAnimBankCollection& C) {
	AnimationBankCollectionExporterState local_s;
	for(KAnimBankCollection::const_iterator bankit = C.begin(); bankit != C.end(); ++bankit) {
		const KAnimBank& B = *bankit->second;
		exportAnimationBank(spriter_data, local_s, bmeta, B);
	}
}

static void exportAnimationBank(xml_node& spriter_data, AnimationBankCollectionExporterState& s, const BuildMetadata& bmeta, const KAnimBank& B) {
	const uint32_t entity_id = s.entity_id++;

	xml_node entity = spriter_data.append_child("entity");

	entity.append_attribute("id") = entity_id;
	entity.append_attribute("name") = B.getName().c_str();

	AnimationBankExporterState local_s;
	for(KAnimBank::const_iterator animit = B.begin(); animit != B.end(); ++animit) {
		const KAnim& A = *animit->second;
		exportAnimation(entity, local_s, bmeta, A);
	}
}

static void exportAnimation(xml_node& entity, AnimationBankExporterState& s, const BuildMetadata& bmeta, const KAnim& A) {
	const uint32_t animation_id = s.animation_id++;

	xml_node animation = entity.append_child("animation");

	animation.append_attribute("id") = animation_id;
	animation.append_attribute("name") = A.getFullName().c_str(); // BUILD_PLAYER ?
	animation.append_attribute("length") = tomilli(A.getDuration());

	xml_node mainline = animation.append_child("mainline");

	AnimationExporterState local_s;
	for(KAnim::framelist_t::const_iterator frameit = A.frames.begin(); frameit != A.frames.end(); ++frameit) {
		const KAnim::Frame& frame = *frameit;
		exportAnimationFrame(mainline, animation, local_s, bmeta, frame);
	}
}

static void exportAnimationFrame(xml_node& mainline, xml_node& animation, AnimationExporterState& s, const BuildMetadata& bmeta, const KAnim::Frame& frame) {
	const uint32_t key_id = s.key_id++;
	const computations_float_type running_length = s.running_length;
	s.running_length += frame.getDuration();

	xml_node mainline_key = mainline.append_child("key");
	mainline_key.append_attribute("id") = key_id;
	mainline_key.append_attribute("time") = tomilli(running_length);

	AnimationFrameExporterState local_s;

	for(KAnim::Frame::elementlist_t::const_reverse_iterator elemit = frame.elements.rbegin(); elemit != frame.elements.rend(); ++elemit) {
		const KAnim::Frame::Element& elem = *elemit;
		BuildMetadata::const_iterator symmeta_match = bmeta.find(elem.getHash());
		if(symmeta_match == bmeta.end()) {
			continue;
		}
		const BuildSymbolMetadata& symmeta = symmeta_match->second;
		exportAnimationFrameElement(mainline_key, animation, local_s, symmeta, elem);
	}
}

/*
 * Decomposes (approximately if this is not possible) a matrix into x/y scalings and a rotation.
 */
template<typename T>
static void decomposeMatrix(T a, T b, T c, T d, T& scale_x, T& scale_y, T& rot) {
	scale_x = sqrt(a*a + b*b);
	if(a < 0) scale_x = -scale_x;

	scale_y = sqrt(c*c + d*d);
	if(d < 0) scale_y = -scale_y;

	rot = atan2(c, d);
}

/*
 * TODO: group together all frames referencing the element as multiple keys in the timeline.
 */
static void exportAnimationFrameElement(xml_node& mainline_key, xml_node& animation, AnimationFrameExporterState& s, const BuildSymbolMetadata& symmeta, const KAnim::Frame::Element& elem) {
	const uint32_t build_frame = elem.getBuildFrame();
	if(build_frame >= symmeta.size()) return;

	const uint32_t timeline_id = s.timeline_id++;
	const uint32_t z_index = s.z_index++;
	const BuildSymbolFrameMetadata& bframemeta = symmeta[build_frame];


	KTools::DataFormatter fmt;

	
	// Sanity checking.
	{
		computations_float_type sort_order = elem.getAnimSortOrder();
		if(sort_order > s.last_sort_order) {
			throw logic_error("State invariant breached: anim sort order progression is not monotone.");
		}
		s.last_sort_order = sort_order;
	}


	/*
	 * Mainline children.
	 */
	
	xml_node object_ref = mainline_key.append_child("object_ref");

	object_ref.append_attribute("id") = timeline_id;
	object_ref.append_attribute("name") = elem.getName().c_str();
	object_ref.append_attribute("folder") = symmeta.folder_id;
	object_ref.append_attribute("file") = build_frame; // This is where deduplication would need to be careful.
	object_ref.append_attribute("abs_x") = 0;
	object_ref.append_attribute("abs_y") = 0;
	object_ref.append_attribute("abs_pivot_x") = bframemeta.pivot_x;
	object_ref.append_attribute("abs_pivot_y") = bframemeta.pivot_y;
	object_ref.append_attribute("abs_angle") = 0; // 360?
	object_ref.append_attribute("abs_scale_x") = 1;
	object_ref.append_attribute("abs_scale_y") = 1;
	object_ref.append_attribute("abs_a") = 1;
	object_ref.append_attribute("timeline") = timeline_id;
	object_ref.append_attribute("key") = 0; // This would need to change on deduplication.
	object_ref.append_attribute("z_index") = z_index;



	/*
	 * Animation children.
	 */

	xml_node timeline = animation.append_child("timeline");

	timeline.append_attribute("id") = timeline_id;
	timeline.append_attribute("name") = fmt("%s_%u_%u", elem.getName().c_str(), unsigned(build_frame), unsigned(timeline_id));



	xml_node timeline_key = timeline.append_child("key");

	timeline_key.append_attribute("id") = 0; // This would need to change on deduplication.
	timeline_key.append_attribute("spin") = 0;



	xml_node object = timeline_key.append_child("object");

	const KAnim::Frame::Element::matrix_type& M = elem.getMatrix();

	KAnim::Frame::Element::matrix_type::projective_vector_type trans;
	M.getTranslation(trans);
	//trans *= SCALE;

	KAnim::float_type scale_x, scale_y, rot;
	decomposeMatrix(M[0][0], M[0][1], M[1][0], M[1][1], scale_x, scale_y, rot);
	if(rot < 0) {
		rot += 2*M_PI;
	}
	rot *= 180.0/M_PI;

	object.append_attribute("folder") = symmeta.folder_id;
	object.append_attribute("file") = build_frame;
	object.append_attribute("x") = trans[0]*SCALE;
	object.append_attribute("y") = -trans[1]*SCALE;
	object.append_attribute("scale_x") = scale_x;
	object.append_attribute("scale_y") = scale_y;
	object.append_attribute("angle") = rot;
}
