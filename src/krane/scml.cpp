#include "scml.hpp"
#include "krane_options.hpp"
#include <pugixml/pugixml.hpp>

#include <limits>

using namespace std;
using namespace Krane;
using namespace pugi;


/***********************************************************************/

// This is the scale applied by the scml compiler in the mod tools.
//static const float_type MODTOOLS_SCALE = KBuild::MODTOOLS_SCALE;

static const float_type TIME_SCALE = 1;

// Tolerance for the square distance of matrices.
static const float_type MATRIX_EPS = 0.5;

/***********************************************************************/


template<typename T>
static inline T normalize_time(T t) CONSTFUNCTION;
template<typename T>
inline T normalize_time(T t) {
	return T(TIME_SCALE)*t;
}

static inline int tomilli(float x) CONSTFUNCTION;
inline int tomilli(float x) {
	return int(ceilf(1000.0f*normalize_time(x)));
}

static inline int tomilli(double x) CONSTFUNCTION;
inline int tomilli(double x) {
	return int(ceil(1000.0*normalize_time(x)));
}


/***********************************************************************/


namespace {
	struct BuildSymbolFrameMetadata {
		float_type pivot_x;
		float_type pivot_y;

		//uint32_t framenum;
		//uint32_t duration;
	};

	struct BuildSymbolMetadata : public std::vector<BuildSymbolFrameMetadata> {
		uint32_t folder_id;
		const KBuild::Symbol* symbol;

		uint32_t getFileId(uint32_t build_framenum) const {
			if(symbol == NULL) {
				throw logic_error("Attempt to access BuildSymbolMetadata object without prior setting of the symbol pointer.");
			}
			return symbol->getFrameIndex(build_framenum);
		}

		const BuildSymbolFrameMetadata& getFrameMetadata(uint32_t build_framenum) const {
			return (*this)[getFileId(build_framenum)];
		}
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
		float_type running_length;
		uint32_t timeline_id;

		AnimationExporterState() : key_id(0), running_length(0), timeline_id(0) {}
	};

	struct AnimationFrameExporterState : public GenericState {
		// This is the same as the current key_id of the outer AnimationExporterState.
		const uint32_t animation_frame_id;

		uint32_t object_ref_id;
		uint32_t z_index;
		float_type last_sort_order;

		AnimationFrameExporterState(uint32_t _animation_frame_id, int max_z_index) : animation_frame_id(_animation_frame_id), object_ref_id(0), z_index(max_z_index) {
			last_sort_order = -numeric_limits<float_type>::infinity();
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
	 * Corresponds to a timeline, listing keys.
	 *
	 * Local to an animation.
	 */
	class AnimationSymbolMetadata : public deque<AnimationSymbolFrameMetadata> {
		xml_node timeline;
		uint32_t id; // timeline id

		int dupeness; // for dupes
		bool is_dupe;

		void setDupedName() {
			DataFormatter fmt;
			timeline.attribute("name") = fmt("%s_%d", timeline.attribute("name").as_string(), dupeness);
		}

	public:
		AnimationSymbolMetadata(int _dupecnt = 0) : timeline(), id(0), dupeness(_dupecnt), is_dupe(_dupecnt > 0) {}

		/*
		 * If no timeline node has been created yet, creates one and sets its attributes.
		 *
		 * Also initializes corresponding element indexed by the elem build frame.
		 */
		void initialize(xml_node animation, uint32_t& current_timeline_id, uint32_t start_time, const KAnim::Frame::Element& elem) {
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

		void setDupe(int _dupeness) {
			if(is_dupe) return;
			dupeness = _dupeness;
			is_dupe = true;
			if(timeline) {
				setDupedName();
			}
		}

		void setDupe() {
			setDupe(dupeness);
		}

		std::string getAnimationName() const {
			if(!timeline) {
				return "";
			}
			return timeline.parent().attribute("name").as_string();
		}

		std::string getName() const {
			if(!timeline) {
				return "";
			}
			return timeline.attribute("name").as_string();
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
	 *
	 * Each value is a list, where a new element is added when a new timeline is required
	 * (due to an object appearing multiple times, such as machine_leg for the researchlab).
	 */
	class AnimationMetadata : public map<const KAnim::Frame::Element*, std::list<AnimationSymbolMetadata>, ptrLess<KAnim::Frame::Element> > {
		xml_node animation;

		// Counts how many times an element hash has been added.
		std::multiset<hash_t> elemhash_mset;

		// Maps an element hash to the first AnimationSymbolMetadata referring to it.
		std::map<hash_t, AnimationSymbolMetadata*> first_comers;

	public:
		typedef value_type::second_type symbolmeta_list;
		typedef symbolmeta_list list_type;

		AnimationMetadata(xml_node _animation) : animation(_animation) {}

		AnimationSymbolMetadata& initializeChild(uint32_t& current_timeline_id, uint32_t start_time, const KAnim::Frame::Element& elem) {
			const int dupeness = int(elemhash_mset.count(elem.getHash()));

			symbolmeta_list& L = (*this)[&elem];

			for(symbolmeta_list::iterator it = L.begin(); it != L.end(); ++it) {
				AnimationSymbolMetadata& animsymmeta = *it;

				assert( ! animsymmeta.empty() );

				if(animsymmeta.back().getStartTime() < start_time) {
					animsymmeta.initialize(animation, current_timeline_id, start_time, elem);
					return animsymmeta;
				}
			}

			L.push_back( AnimationSymbolMetadata(dupeness) );
			AnimationSymbolMetadata& animsymmeta = L.back();

			if(dupeness == 0) {
				first_comers.insert( std::make_pair(elem.getHash(), &animsymmeta) );
			}
			else if(dupeness == 1) {
				AnimationSymbolMetadata* first_animsymmeta = first_comers[elem.getHash()];
				if(first_animsymmeta == NULL) {
					throw std::logic_error("first_comers mapped to NULL, but elemhash_mset has a count greater than 0.");
				}
				first_animsymmeta->setDupe();
			}
			animsymmeta.initialize(animation, current_timeline_id, start_time, elem);

			elemhash_mset.insert(elem.getHash());

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

static void exportAnimationFrameElement(xml_node mainline_key, AnimationFrameExporterState& s, AnimationSymbolMetadata& animsymmeta, const BuildSymbolMetadata& symmeta, const KAnim::Frame::Element& elem);

static void exportAnimationSymbolTimeline(const BuildSymbolMetadata& symmeta, const AnimationSymbolMetadata& animsymmeta);


/***********************************************************************/


void Krane::exportToSCML(std::ostream& out, const KBuild& bild, const KAnimBankCollection& banks) {
	BinIOHelper::sanitizeStream(out);

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

	//cout << "Exporting build symbol " << sym.getName() << endl;

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
	symmeta.symbol = &sym;
}

static void exportBuildSymbolFrame(xml_node folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame) {
	const uint32_t file_id = s.file_id++;

	xml_node file = folder.append_child("file");

	//cout << "Exporting build symbol frame #" << file_id << ": " << frame.getUnixPath() << endl;

	const KBuild::Symbol::Frame::bbox_type& bbox = frame.getBoundingBox();

	float_type x, y;
	int w, h;
	x = bbox.x();
	y = bbox.y();
	w = bbox.int_w();
	h = bbox.int_h();

	float_type pivot_x = 0.5 - x/w;
	float_type pivot_y = 0.5 + y/h;

	framemeta.pivot_x = pivot_x;
	framemeta.pivot_y = pivot_y;
	//framemeta.framenum = frame.getFrameNumber();
	//framemeta.duration = frame.getDuration();

	file.append_attribute("id") = file_id;
	file.append_attribute("name") = frame.getUnixPath().c_str();
	file.append_attribute("width") = w;///MODTOOLS_SCALE;
	file.append_attribute("height") = h;///MODTOOLS_SCALE;
	//file.append_attribute("original_width") = w/MODTOOLS_SCALE;
	//file.append_attribute("original_height") = h/MODTOOLS_SCALE;
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

	//cout << "Exporting animation: " << A.getName() << endl;

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

	// Put an end frame.
	if(!A.frames.empty()) {
		const KAnim::Frame& frame = A.frames.back();
		exportAnimationFrame(mainline, local_s, animmeta, bmeta, frame);
	}

	for(AnimationMetadata::const_iterator animsymlistit = animmeta.begin(); animsymlistit != animmeta.end(); ++animsymlistit) {
		BuildMetadata::const_iterator symmeta_match = bmeta.find(animsymlistit->first->getHash());

		if(symmeta_match == bmeta.end()) {
			throw logic_error("Program logic state invariant breached: hashes of keys of AnimationMetadata are not a subset of keys of BuildMetadata.");
		}

		const AnimationMetadata::list_type& animsymlist = animsymlistit->second;

		for(AnimationMetadata::list_type::const_iterator animsymit = animsymlist.begin(); animsymit != animsymlist.end(); ++animsymit) {
			exportAnimationSymbolTimeline(symmeta_match->second, *animsymit);
		}
	}
}
static void exportAnimationFrame(xml_node mainline, AnimationExporterState& s, AnimationMetadata& animmeta, const BuildMetadata& bmeta, const KAnim::Frame& frame) {
	const uint32_t key_id = s.key_id++;
	const float_type running_length = s.running_length;
	s.running_length += frame.getDuration();
	uint32_t start_time = tomilli(running_length);

	xml_node mainline_key = mainline.append_child("key");
	mainline_key.append_attribute("id") = key_id;
	mainline_key.append_attribute("time") = start_time;

	//cout << "Exporting animation frame " << key_id << endl;

	AnimationFrameExporterState local_s(key_id, int(frame.elements.size()));

	for(KAnim::Frame::elementlist_t::const_iterator elemit = frame.elements.begin(); elemit != frame.elements.end(); ++elemit) {
		const KAnim::Frame::Element& elem = *elemit;

		BuildMetadata::const_iterator symmeta_match = bmeta.find(elem.getHash());
		if(symmeta_match == bmeta.end()) {
			continue;
		}

		const BuildSymbolMetadata& symmeta = symmeta_match->second;

		AnimationSymbolMetadata& animsymmeta = animmeta.initializeChild(s.timeline_id, start_time, elem);

		exportAnimationFrameElement(mainline_key, local_s, animsymmeta, symmeta, elem);
	}
}

static void exportAnimationFrameElement(xml_node mainline_key, AnimationFrameExporterState& s, AnimationSymbolMetadata& animsymmeta, const BuildSymbolMetadata& symmeta, const KAnim::Frame::Element& elem) {
	const uint32_t object_ref_id = s.object_ref_id++;
	const uint32_t z_index = s.z_index--;

	const uint32_t build_frame = elem.getBuildFrame();

	const BuildSymbolFrameMetadata& bframemeta = symmeta.getFrameMetadata(build_frame);

	//const uint32_t build_frame = elem.getBuildFrame();


	// Sanity checking.
	{
		float_type sort_order = elem.getAnimSortOrder();
		if(sort_order < s.last_sort_order) {
			throw logic_error("Program logic state invariant breached: anim sort order progression is not monotone.");
		}
		s.last_sort_order = sort_order;
	}

	//cout << "Exporting animation frame element " << elem.getName() << ", build frame " << elem.getBuildFrame() << ", key id " << animsymmeta.getTimelineId() << endl;


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
	object_ref.append_attribute("abs_pivot_x") = bframemeta.pivot_x;//*MODTOOLS_SCALE;
	object_ref.append_attribute("abs_pivot_y") = bframemeta.pivot_y;//*MODTOOLS_SCALE;
	object_ref.append_attribute("abs_angle") = 0; // 360?
	object_ref.append_attribute("abs_scale_x") = 1;
	object_ref.append_attribute("abs_scale_y") = 1;
	object_ref.append_attribute("abs_a") = 1;
	object_ref.append_attribute("timeline") = animsymmeta.getTimelineId();
	object_ref.append_attribute("key") = animsymmeta.getKeyId();//build_frame; // This changed for deduplication.
	object_ref.append_attribute("z_index") = z_index;
}


struct matrix_components {
	

	float_type scale_x, scale_y;
	float_type angle;
	int spin;

	matrix_components() :
		scale_x(1),
		scale_y(1),
		angle(0),
		spin(1)
	{}
};

// sup entry-wise norm
template<typename MatrixTypeA, typename MatrixTypeB>
static typename MatrixTypeA::value_type matrix_dist(const MatrixTypeA& A, const MatrixTypeB& B) {
	typedef typename MatrixTypeA::value_type f_t;

	f_t max_delta = 0;
	for(int i = 0; i < 2; i++) {
		for(int j = 0; j < 2; j++) {
			const f_t delta = fabs(A[i][j] - B[i][j]);
			if(delta > max_delta) {
				max_delta = delta;
			}
		}
	}

	return max_delta;
}

static SquareMatrix<2, float_type> rotationMatrix(float_type theta) {
	typedef float_type f_t;

	const f_t c = cos(theta);
	const f_t s = sin(theta);

	SquareMatrix<2, f_t> M = nil;
	M[0][0] = c;
	M[0][1] = -s;
	M[1][0] = s;
	M[1][1] = c;

	return M;
}

static SquareMatrix<2, float_type> scaleMatrix(float_type x, float_type y) {
	typedef float_type f_t;
	
	SquareMatrix<2, f_t> M;
	assert(M[0][1] == 0 && M[1][0] == 0);

	M[0][0] = x;
	M[1][1] = y;

	return M;
}

/*
 * Decomposes (approximately if this is not possible exactly) a matrix into xy scalings and a rotation angle in radians.
 *
 * In Spriter, rotation is applied before scaling. Counter-intuitive, huh?
 *
 * The result is stored in ret, and last holds the last batch of results
 * (the ones relevant for further computation, anyway)
 */
template<typename MatrixType>
static void decomposeMatrix(const MatrixType& M, matrix_components& ret, matrix_components& last, bool& is_first) {
	static const float_type eps = 1e-5;

	ret.scale_x = sqrt(M[0][0]*M[0][0] + M[0][1]*M[0][1]);
	ret.scale_y = sqrt(M[1][0]*M[1][0] + M[1][1]*M[1][1]);

	const float_type det = M[0][0]*M[1][1] - M[1][0]*M[0][1];
	if(det < 0) {
		if(is_first || last.scale_x <= last.scale_y) {
			ret.scale_x = -ret.scale_x;
			is_first = false;
		}
		else {
			ret.scale_y = -ret.scale_y;
		}
	}

	if(fabs(ret.scale_x) < eps || fabs(ret.scale_y) < eps) {
		ret.angle = last.angle;
	}
	else {
		using namespace std;
		const float_type sin_approx = 0.5*( M[1][0]/ret.scale_y - M[0][1]/ret.scale_x );
		const float_type cos_approx = 0.5*( M[0][0]/ret.scale_x + M[1][1]/ret.scale_y );
		ret.angle = atan2(sin_approx, cos_approx);
	}

	ret.spin = (fabs(ret.angle - last.angle) <= M_PI ? 1 : -1);
	if(ret.angle < last.angle) {
		ret.spin = -ret.spin;
	}

	last.scale_x = ret.scale_x;
	last.scale_y = ret.scale_y;
	last.angle = ret.angle;
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

	/*
	 * Stores the last values of the matrix decomposition.
	 *
	 * Used to preserve continuity and minimize oscillations.
	 *
	 * It is edited internally by the decomposition function.
	 */
	matrix_components last_result;
	last_result.angle = 0;

	bool is_first = true;

	/*
	 * Stores the maximum square "badness" of the symbol, over all its frames.
	 *
	 * Only used when the --check-animation-fidelity option is given.
	 */
	Maybe<float_type> symbol_badness;

	int key_id = 0;
	for(AnimationSymbolMetadata::const_iterator animsymframeiter = animsymmeta.begin(); animsymframeiter != animsymmeta.end(); ++animsymframeiter) {
		const AnimationSymbolFrameMetadata& animsymframemeta = *animsymframeiter;

		xml_node timeline_key = timeline.append_child("key");

		timeline_key.append_attribute("id") = key_id++;//build_frame; // This changed for deduplication.
		timeline_key.append_attribute("time") = animsymframemeta.getStartTime();//tomilli(frame_duration*bframemeta.duration);//tomilli(frame_duration*bframemeta.duration);


		const matrix_type& M = animsymframemeta.getMatrix();

		matrix_type::projective_vector_type trans;
		M.getTranslation(trans);

		matrix_components geo;
		decomposeMatrix(M, geo, last_result, is_first);


		if(options::check_animation_fidelity) {
			float_type md = matrix_dist(M, scaleMatrix(geo.scale_x, geo.scale_y)*rotationMatrix(geo.angle));
			if(md > MATRIX_EPS && (symbol_badness == nil || symbol_badness.value() < md)) {
				symbol_badness = Just(md);
			}
		}


		float_type angle = geo.angle;
		if(angle < 0) {
			angle += 2*M_PI;
		}
		angle *= 180.0/M_PI;


		timeline_key.append_attribute("spin") = geo.spin;


		xml_node object = timeline_key.append_child("object");

		object.append_attribute("folder") = symmeta.folder_id;
		object.append_attribute("file") = symmeta.getFileId(animsymframemeta.getBuildFrame());//build_frame;//animsymframemeta.getBuildFrame();
		object.append_attribute("x") = trans[0];
		object.append_attribute("y") = -trans[1];
		object.append_attribute("scale_x") = geo.scale_x;
		object.append_attribute("scale_y") = geo.scale_y;
		object.append_attribute("angle") = angle;
	}

	if(symbol_badness != nil) {
		cerr << "WARNING: the geometry of animation symbol \"" << animsymmeta.getName() << "\" in animation \"" << animsymmeta.getAnimationName() << "\" is not fully representable in Spriter. Expect lower animation fidelity. (max matrix distance: " << symbol_badness.value() << ")" << endl;
	}
}
