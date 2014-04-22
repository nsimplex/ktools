#include "scml.hpp"
#include <pugixml/pugixml.hpp>

using namespace std;
using namespace Krane;
using namespace pugi;


/***********************************************************************/

// Global linear scale (applied to dimensions, translations, etc.).
static const computations_float_type SCALE = 1;


/***********************************************************************/


static void sanitize_stream(std::ostream& out) {
	out.imbue(locale::classic());
}


/***********************************************************************/


struct BuildSymbolFrameMetadata {
	computations_float_type pivot_x;
	computations_float_type pivot_y;
};

struct BuildSymbolMetadata : public std::vector<BuildSymbolFrameMetadata> {
	int folder_id;
};

typedef map<hash_t, BuildSymbolMetadata> BuildMetadata;


struct GenericState {
//	KTools::DataFormatter fmt;
};

struct BuildExporterState : public GenericState {
	int folder_id;

	BuildExporterState() : folder_id(0) {}
};

struct BuildSymbolExporterState : public GenericState {
	int file_id;

	BuildSymbolExporterState() : file_id(0) {}
};


/***********************************************************************/


static void exportBuild(xml_node& spriter_data, BuildMetadata& bmeta, const KBuild& bild);

static void exportBuildSymbol(xml_node& spriter_data, BuildExporterState& s, BuildSymbolMetadata& symmeta, const KBuild::Symbol& sym);

static void exportBuildSymbolFrame(xml_node& folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame);


static void exportAnimationBankCollection(xml_node& spriter_data, const BuildMetadata& bmeta, const KAnimBankCollection& A);


/***********************************************************************/


void Krane::exportToSCML(std::ostream& out, const KBuild& bild, const KAnimBankCollection& banks) {
	sanitize_stream(out);

	(void)banks;

	xml_document scml;
	xml_node decl = scml.prepend_child(node_declaration);
	decl.append_attribute("encoding") = "UTF-8";

	xml_node spriter_data = scml.append_child("spriter_data");

	spriter_data.append_attribute("scml_version") = "1.0";
	spriter_data.append_attribute("generator") = "BrashMonkey Spriter";
	spriter_data.append_attribute("generator_version") = "b5";

	BuildMetadata bmeta;
	exportBuild(spriter_data, bmeta, bild);

	xml_node entity = spriter_data.append_child("entity");

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

	const int folder_id = s.folder_id++;

	xml_node folder = spriter_data.append_child("folder");

	folder.append_attribute("id") = folder_id;
	folder.append_attribute("name") = sym.getUnixPath().c_str();

	BuildSymbolExporterState local_s;

	symmeta.resize(sym.frames.size());
	size_t framenum = 0;
	for(frame_iterator it = sym.frames.begin(); it != sym.frames.end(); ++it) {
		const KBuild::Symbol::Frame& frame = *it;
		exportBuildSymbolFrame(folder, local_s, symmeta[framenum], frame);
		framenum++;
	}

	symmeta.folder_id = folder_id;
}

static void exportBuildSymbolFrame(xml_node& folder, BuildSymbolExporterState& s, BuildSymbolFrameMetadata& framemeta, const KBuild::Symbol::Frame& frame) {
	typedef KBuild::float_type float_type;

	const int file_id = s.file_id++;

	xml_node file = folder.append_child("file");

	float_type x, y, w, h;
	x = frame.bbox.x();
	y = frame.bbox.y();
	w = frame.bbox.w();
	h = frame.bbox.h();

	computations_float_type pivot_x = 0.5 - (x/w)*SCALE;
	computations_float_type pivot_y = 0.5 + (y/h)*SCALE;

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


