#include "scml.hpp"
#include <pugixml/pugixml.hpp>

#include <limits>

using namespace std;
using namespace Krane;
using namespace pugi;


/***********************************************************************/

// This is the scale applied by the scml compiler in the mod tools.
static const computations_float_type MODTOOLS_SCALE = KBuild::MODTOOLS_SCALE;


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

		// Animation frame number.
		uint32_t anim_framenum;
		uint32_t duration;
	};

	struct BuildSymbolMetadata : public std::vector<BuildSymbolFrameMetadata> {
		uint32_t folder_id;

		// Timeline id in
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
		uint32_t timeline_id;

		AnimationExporterState() : key_id(0), running_length(0), timeline_id(0) {}
	};

	struct AnimationFrameExporterState : public GenericState {
		// This is the same as the current key_id of the outer AnimationExporterState.
		const uint32_t animation_frame_id;

		uint32_t object_ref_id;
		uint32_t z_index;
		computations_float_type last_sort_order;

		AnimationFrameExporterState(uint32_t _animation_frame_id) : animation_frame_id(_animation_frame_id), object_ref_id(0), z_index(0) {
			last_sort_order = numeric_limits<computations_float_type>::infinity();
		}
	};

	/***********/

	/*
	 * Metadata on a single occurrence of a build symbol frame
	 * (essentially, a key/object within a timeline)
	 */
	class AnimationSymbolFrameMetadata {
	public:
		typedef KAnim::Frame::Element::matrix_type matrix_type;

	private:
		const matrix_type* M;
		uint32_t build_frame;
		uint32_t start_time; // milli

	public:
		AnimationSymbolFrameMetadata() : M(NULL) {}
		AnimationSymbolFrameMetadata(const KAnim::Frame::Element& elem, uint32_t _start_time) : M(NULL) {
			initialize(elem, _start_time);
		}

		void initialize(const KAnim::Frame::Element& elem, uint32_t _start_time) {
			if(M != NULL) {
				throw logic_error( ("Reinitializing AnimationSymbolFrameMetadata for frame element " + elem.getName() + ".").c_str() );
			}
			M = &elem.getMatrix();
			build_frame = elem.getBuildFrame();
			start_time = _start_time;
		}

		const matrix_type& getMatrix() const {
			if(M == NULL) {
				throw logic_error("Attempt to fetch matrix from uninitialized AnimationSymbolFrameMetadata.");
			}
			return *M;
		}

		uint32_t getBuildFrame() const {
			return build_frame;
		}

		uint32_t getStartTime() const {
			return start_time;
		}
	};

	/*
	 * Metadata on build symbols as animation frame elements.
	 * Keys are the animation frame ids.
	 *
	 * Local to an animation.
	 */
	class AnimationSymbolMetadata : public list<AnimationSymbolFrameMetadata> {
		xml_node timeline;
		uint32_t id; // timeline id

		int dupeness; // for dupes
		bool is_dupe;

		void setDupedName() {
			DataFormatter fmt;
			timeline.attribute("name") = fmt("%s_%d", timeline.attribute("name").as_string(), dupeness);
		}

	public:
		AnimationSymbolMetadata(int _dupecnt) : timeline(), id(0), dupeness(_dupecnt), is_dupe(_dupecnt > 0) {}

		/*
		 * If no timeline node has been created yet, creates one and sets its attributes.
		 *
		 * Also initializes corresponding element indexed by the elem build frame.
		 */
		void initialize(xml_node animation, uint32_t& current_timeline_id, uint32_t start_time, const BuildSymbolFrameMetadata& bframemeta, const KAnim::Frame::Element& elem) {
			//assert(anim_frame_id < size());
			if(!timeline) {
				id = current_timeline_id++;
				timeline = animation.append_child("timeline");
				timeline.append_attribute("id") = id;
				timeline.append_attribute("name") = elem.getName().c_str();

				if(is_dupe) {
					setDupedName();
				}
			}
			push_back( AnimationSymbolFrameMetadata(elem, start_time) );
		}

		void setDupe() {
			if(is_dupe) return;
			is_dupe = true;
			if(timeline) {
				setDupedName();
			}
		}

		xml_node getTimeline() const {
			if(!timeline) {
				throw logic_error("Attempt to fetch timeline node from uninitialized AnimationSymbolMetadata object.");
			}
			return timeline;
		}

		uint32_t getTimelineId() const {
			if(!timeline) {
				throw logic_error("Attempt to fetch timeline id from uninitialized AnimationSymbolMetadata object.");
			}
			return id;
		}

		int getKeyId() const {
			return int(size()) - 1;
		}
	};

	/*
	 * Maps build symbol hashes to their metadata (as animation frame elements).
	 */
	class AnimationMetadata : public map<hash_t, list<AnimationSymbolMetadata> > {
		xml_node animation;

	public:
		typedef list<AnimationSymbolMetadata> symbolmeta_list;
		typedef symbolmeta_list list_type;

		AnimationMetadata(xml_node _animation) : animation(_animation) {}

		AnimationSymbolMetadata& initializeChild(hash_t symbolHash, uint32_t& current_timeline_id, uint32_t start_time, const BuildSymbolFrameMetadata& bframemeta, const KAnim::Frame::Element& elem) {
			symbolmeta_list& L = (*this)[symbolHash];

			const int dupeness = int(L.size());

			for(symbolmeta_list::iterator it = L.begin(); it != L.end(); ++it) {
				AnimationSymbolMetadata& animsymmeta = *it;

				assert( ! animsymmeta.empty() );

				if(animsymmeta.back().getStartTime() < start_time) {
					animsymmeta.initialize(animation, current_timeline_id, start_time, bframemeta, elem);
					return animsymmeta;
				}
			}

			L.push_back( AnimationSymbolMetadata(dupeness) );
			AnimationSymbolMetadata& animsymmeta = L.back();
			animsymmeta.initialize(animation, current_timeline_id, start_time, bframemeta, elem);

			if(dupeness > 0) {
				L.front().setDupe();
			}

			return animsymmeta;
		}
	};
}


/***********************************************************************/


static void exportBuild(xml_node spriter_data, BuildMetadata& bmeta, const KBuild& bild);

static void exportBuildSymbol(xml_node spriter_data, BuildExporterState& s, BuildSymbolMetadata& symmeta, const KBuild::Symbol& sym);

static void exportBuildSymbolFrame(xml_node folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame);


static void exportAnimationBankCollection(xml_node spriter_data, const BuildMetadata& bmeta, const KAnimBankCollection& C);

static void exportAnimationBank(xml_node spriter_data, AnimationBankCollectionExporterState& s, const BuildMetadata& bmeta, const KAnimBank& B);

static void exportAnimation(xml_node entity, AnimationBankExporterState& s, const BuildMetadata& bmeta, const KAnim& A);

static void exportAnimationFrame(xml_node mainline, AnimationExporterState& s, AnimationMetadata& animmeta, const BuildMetadata& bmeta, const KAnim::Frame& frame);

static void exportAnimationFrameElement(xml_node mainline_key, AnimationFrameExporterState& s, AnimationSymbolMetadata& animsymmeta, const BuildSymbolFrameMetadata& bframemeta, const KAnim::Frame::Element& elem);

static void exportAnimationSymbolTimeline(const BuildSymbolMetadata& symmeta, const AnimationSymbolMetadata& animsymmeta);


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

static void exportBuild(xml_node spriter_data, BuildMetadata& bmeta, const KBuild& bild) {
	typedef KBuild::symbolmap_const_iterator symbolmap_iterator;

	BuildExporterState local_s;

	for(symbolmap_iterator it = bild.symbols.begin(); it != bild.symbols.end(); ++it) {
		hash_t h = it->first;
		const KBuild::Symbol& sym = it->second;
		exportBuildSymbol(spriter_data, local_s, bmeta[h], sym);
	}
}

static void exportBuildSymbol(xml_node spriter_data, BuildExporterState& s, BuildSymbolMetadata& symmeta, const KBuild::Symbol& sym) {
	typedef KBuild::frame_const_iterator frame_iterator;

	const uint32_t folder_id = s.folder_id++;

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

static void exportBuildSymbolFrame(xml_node folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame) {
	typedef KBuild::float_type float_type;

	const uint32_t file_id = s.file_id++;

	xml_node file = folder.append_child("file");

	const KBuild::Symbol::Frame::bbox_type& bbox = frame.getBoundingBox();

	float_type x, y;
	int w, h;
	x = bbox.x();
	y = bbox.y();
	w = bbox.int_w();
	h = bbox.int_h();

	computations_float_type pivot_x = 0.5 - x/w;
	computations_float_type pivot_y = 0.5 + y/h;

	framemeta.pivot_x = pivot_x;
	framemeta.pivot_y = pivot_y;
	framemeta.anim_framenum = frame.getAnimationFrameNumber();
	framemeta.duration = frame.getDuration();

	file.append_attribute("id") = file_id;
	file.append_attribute("name") = frame.getUnixPath().c_str();
	file.append_attribute("width") = w;
	file.append_attribute("height") = h;
	file.append_attribute("original_width") = w/MODTOOLS_SCALE;
	file.append_attribute("original_height") = h/MODTOOLS_SCALE;
	file.append_attribute("pivot_x") = pivot_x;
	file.append_attribute("pivot_y") = pivot_y;
}

/***********************************************************************/

static void exportAnimationBankCollection(xml_node spriter_data, const BuildMetadata& bmeta, const KAnimBankCollection& C) {
	AnimationBankCollectionExporterState local_s;
	for(KAnimBankCollection::const_iterator bankit = C.begin(); bankit != C.end(); ++bankit) {
		const KAnimBank& B = *bankit->second;
		exportAnimationBank(spriter_data, local_s, bmeta, B);
	}
}

static void exportAnimationBank(xml_node spriter_data, AnimationBankCollectionExporterState& s, const BuildMetadata& bmeta, const KAnimBank& B) {
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

static void exportAnimation(xml_node entity, AnimationBankExporterState& s, const BuildMetadata& bmeta, const KAnim& A) {
	const uint32_t animation_id = s.animation_id++;
	const computations_float_type frame_duration = A.getFrameDuration();

	xml_node animation = entity.append_child("animation");
	AnimationMetadata animmeta(animation);

	animation.append_attribute("id") = animation_id;
	animation.append_attribute("name") = A.getFullName().c_str(); // BUILD_PLAYER ?
	animation.append_attribute("length") = tomilli(A.getDuration());

	xml_node mainline = animation.append_child("mainline");

	AnimationExporterState local_s;
	for(KAnim::framelist_t::const_iterator frameit = A.frames.begin(); frameit != A.frames.end(); ++frameit) {
		const KAnim::Frame& frame = *frameit;
		exportAnimationFrame(mainline, local_s, animmeta, bmeta, frame);
	}

	for(AnimationMetadata::const_iterator animsymlistit = animmeta.begin(); animsymlistit != animmeta.end(); ++animsymlistit) {
		BuildMetadata::const_iterator symmeta_match = bmeta.find(animsymlistit->first);

		if(symmeta_match == bmeta.end()) {
			throw logic_error("Program logic state invariant breached: keys of AnimationMetadata are not a subset of keys of BuildMetadata.");
		}

		const AnimationMetadata::list_type& animsymlist = animsymlistit->second;

		for(AnimationMetadata::list_type::const_iterator animsymit = animsymlist.begin(); animsymit != animsymlist.end(); ++animsymit) {
			exportAnimationSymbolTimeline(symmeta_match->second, *animsymit);
		}
	}
}

static void exportAnimationFrame(xml_node mainline, AnimationExporterState& s, AnimationMetadata& animmeta, const BuildMetadata& bmeta, const KAnim::Frame& frame) {
	const uint32_t key_id = s.key_id++;
	const computations_float_type running_length = s.running_length;
	s.running_length += frame.getDuration();
	uint32_t start_time = tomilli(running_length);

	xml_node mainline_key = mainline.append_child("key");
	mainline_key.append_attribute("id") = key_id;
	mainline_key.append_attribute("time") = start_time;

	AnimationFrameExporterState local_s(key_id);

	for(KAnim::Frame::elementlist_t::const_reverse_iterator elemit = frame.elements.rbegin(); elemit != frame.elements.rend(); ++elemit) {
		const KAnim::Frame::Element& elem = *elemit;

		BuildMetadata::const_iterator symmeta_match = bmeta.find(elem.getHash());
		if(symmeta_match == bmeta.end()) {
			continue;
		}

		const BuildSymbolMetadata& symmeta = symmeta_match->second;

		const uint32_t build_frame = elem.getBuildFrame();
		if(build_frame >= symmeta.size()) {
			continue;
		}

		const BuildSymbolFrameMetadata& bframemeta = symmeta[build_frame];

		AnimationSymbolMetadata& animsymmeta = animmeta.initializeChild(elem.getHash(), s.timeline_id, start_time, bframemeta, elem);

		exportAnimationFrameElement(mainline_key, local_s, animsymmeta, bframemeta, elem);
	}
}

static void exportAnimationFrameElement(xml_node mainline_key, AnimationFrameExporterState& s, AnimationSymbolMetadata& animsymmeta, const BuildSymbolFrameMetadata& bframemeta, const KAnim::Frame::Element& elem) {
	const uint32_t object_ref_id = s.object_ref_id++;
	const uint32_t z_index = s.z_index++;

	//const uint32_t build_frame = elem.getBuildFrame();


	// Sanity checking.
	{
		computations_float_type sort_order = elem.getAnimSortOrder();
		if(sort_order > s.last_sort_order) {
			throw logic_error("Program logic state invariant breached: anim sort order progression is not monotone.");
		}
		s.last_sort_order = sort_order;
	}


	xml_node object_ref = mainline_key.append_child("object_ref");

	object_ref.append_attribute("id") = object_ref_id;
	object_ref.append_attribute("name") = elem.getName().c_str();
	/*
	 * These are now defined only in the <object> within the timeline.
	 *
	object_ref.append_attribute("folder") = symmeta.folder_id;
	object_ref.append_attribute("file") = build_frame; // This is where deduplication would need to be careful.
	*/
	object_ref.append_attribute("abs_x") = 0;
	object_ref.append_attribute("abs_y") = 0;
	object_ref.append_attribute("abs_pivot_x") = bframemeta.pivot_x;
	object_ref.append_attribute("abs_pivot_y") = bframemeta.pivot_y;
	object_ref.append_attribute("abs_angle") = 0; // 360?
	object_ref.append_attribute("abs_scale_x") = 1;
	object_ref.append_attribute("abs_scale_y") = 1;
	object_ref.append_attribute("abs_a") = 1;
	object_ref.append_attribute("timeline") = animsymmeta.getTimelineId();
	object_ref.append_attribute("key") = animsymmeta.getKeyId();//build_frame; // This changed for deduplication.
	object_ref.append_attribute("z_index") = z_index;
}

/*
 * Decomposes (approximately if this is not possible exactly) a matrix into xy scalings and a rotation angle in radians.
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
 * Exports the corresponding animation timeline of a build symbol.
 */
static void exportAnimationSymbolTimeline(const BuildSymbolMetadata& symmeta, const AnimationSymbolMetadata& animsymmeta) {
	// 3x3
	typedef AnimationSymbolFrameMetadata::matrix_type matrix_type;

	/*
	 * Animation children.
	 */

	xml_node timeline = animsymmeta.getTimeline();

	int key_id = 0;
	for(AnimationSymbolMetadata::const_iterator animsymframeiter = animsymmeta.begin(); animsymframeiter != animsymmeta.end(); ++animsymframeiter) {
		const AnimationSymbolFrameMetadata& animsymframemeta = *animsymframeiter;

		xml_node timeline_key = timeline.append_child("key");

		timeline_key.append_attribute("id") = key_id++;//build_frame; // This changed for deduplication.
		timeline_key.append_attribute("time") = animsymframemeta.getStartTime();//tomilli(frame_duration*bframemeta.duration);//tomilli(frame_duration*bframemeta.duration);
		timeline_key.append_attribute("spin") = 0;


			xml_node object = timeline_key.append_child("object");

			const matrix_type& M = animsymframemeta.getMatrix();

			matrix_type::projective_vector_type trans;
			M.getTranslation(trans);

			computations_float_type scale_x, scale_y, rot;
			decomposeMatrix(M[0][0], M[0][1], M[1][0], M[1][1], scale_x, scale_y, rot);
			if(rot < 0) {
				rot += 2*M_PI;
			}
			rot *= 180.0/M_PI;

			object.append_attribute("folder") = symmeta.folder_id;
			object.append_attribute("file") = animsymframemeta.getBuildFrame();//build_frame;//animsymframemeta.getBuildFrame();
			object.append_attribute("x") = trans[0];
			object.append_attribute("y") = -trans[1];
			object.append_attribute("scale_x") = scale_x;
			object.append_attribute("scale_y") = scale_y;
			object.append_attribute("angle") = rot;
	}
}
