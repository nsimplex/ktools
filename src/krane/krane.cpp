#include "krane.hpp"
#include "ktex/ktex.hpp"

#include <clocale>

using namespace Krane;
using namespace std;

typedef std::list<KTools::VirtualPath> pathlist_t;

Magick::Image load_image(const Compat::Path& path) {
	if(!path.exists()) {
		throw(Error("The path " + path + " does not exist."));
	}

	if(path.hasExtension("tex")) {
		KTEX::File ktex;
		ktex.loadFrom(path, 0);
		return ktex.Decompress();
	}
	else {
		Magick::Image img;
		MAGICK_WRAP(img.read(path));
		return img;
	}
}

static void preprocess_input_list(pathlist_t& inputs) {
	typedef pathlist_t::iterator iter_t;
	typedef pathlist_t::value_type path_t;

	for(iter_t it = inputs.begin(); it != inputs.end();) {
		if(!it->exists()) {
			cerr << "Input path `" << *it << "' does not exist, skipping..." << endl;
			inputs.erase(it++);
			continue;
		}

		if(it->isDirectory()) {
			path_t build = (*it)/"build.bin";
			path_t anim = (*it)/"anim.bin";

			if(build.exists()) {
				inputs.insert(it, build);
			}
			if(anim.exists()) {
				inputs.insert(it, anim);
			}

			inputs.erase(it++);
		}
		else if(it->hasExtension("tex")) {
			cerr << "Skipping TEX file `" << *it << "'..." << endl;
			inputs.erase(it++);
		}
		else {
			++it;
		}
	}

	if(inputs.empty()) {
		throw KToolsError("No input files.");
	}
}

static std::string append_sentence(const std::string& original, const std::string& sentence) {
	std::string ret = original;
	if(ret.length() > 0 && ret[ret.length() - 1] != '.') {
		ret.append('.', 1);
	}
	ret.append(sentence);
	return ret;
}

static KBuild* load_build(std::istream& in, const pathlist_t::value_type& inputdir) {
	typedef KBuild::atlaslist_t::iterator atlasiter_t;

	KBuildFile bildfile;

	try {
		bildfile.load(in, options::verbosity);
	}
	catch(std::exception& e) {
		cerr << "ERROR: " << append_sentence(e.what(), "Skipping build file.") << endl;
		return NULL;
	}

	KBuild* bild = bildfile.getBuild();
	if(options::allowed_build != nil && bild->getName() != options::allowed_build.value()) {
		return NULL;
	}

	for(atlasiter_t it = bild->atlases.begin(); it != bild->atlases.end(); ++it) {
		it->second = load_image( inputdir/it->first );
	}

	bildfile.softClear();
	return bild;
}

static void load_anims(std::istream& in, KAnimBankCollection& banks) {
	typedef KAnimFile<> animfile_t;

	animfile_t animfile;

	try {
		animfile.load(in, options::verbosity);
	}
	catch(std::exception& e) {
		cerr << "ERROR: " << append_sentence(e.what(), "Skipping animation file.") << endl;
		return;
	}

	banks.addAnims( animfile.anims.begin(), animfile.anims.end() );

	animfile.softClear();
}

static KBuild* process_input_list(const pathlist_t& inputs, KAnimBankCollection& banks) {
	typedef pathlist_t::const_iterator iter_t;
	typedef pathlist_t::value_type path_t;

	KBuild* bild = NULL;

	for(iter_t it = inputs.begin(); it != inputs.end(); ++it) {
		std::istream* in = it->openIn(std::istream::binary);
		check_stream_validity(*in, *it);

		const uint32_t magic_number = BinIOHelper::getMagicNumber(*in);
		if(magic_number == KBuildFile::MAGIC_NUMBER) {
			if(options::verbosity >= 0) {
				cout << "Loading build from `" << *it << "'..." << endl;
			}
			KBuild* newbild = load_build(*in, it->dirname());
			if(newbild == NULL) {
				if(options::verbosity >= 0) {
					cout << "Discarded." << endl;
				}
			}
			else if(bild == NULL) {
				bild = newbild;
			}
			else {
				delete in;
				delete bild;
				throw KToolsError("Multiple builds found in the input. Restrict the choice by specifying a build name.");
			}
		}
		else if(magic_number == GenericKAnimFile::MAGIC_NUMBER) {
			if(options::verbosity >= 0) {
				cout << "Loading anims from `" << *it << "'..." << endl;
			}
			load_anims(*in, banks);
		}
		else {
			delete in;
			delete bild;
			throw KToolsError("Input file `" + *it + "' has unknown type.");
		}

		delete in;
	}

	return bild;
}

static void configure_bank_collection(KAnimBankCollection& banks) {
	if(options::banks_rename != nil) {
		KAnimBankRenamer* f = new KAnimBankRenamer(options::banks_rename);
		banks.setMapper(f);
	}

	if(options::allowed_banks.size() > 0) {
		KAnimBankSelector* s = new KAnimBankSelector;
		for(options::allowed_banks_t::const_iterator it = options::allowed_banks.begin(); it != options::allowed_banks.end(); ++it) {
			s->addBank(*it);
		}
		banks.setSelector(s);
	}
}

int main(int argc, char* argv[]) {
	typedef std::vector< std::pair<Compat::Path, Magick::Image> > imglist_t;

	Magick::InitializeMagick(argv[0]);

	pathlist_t input_paths;
	Compat::Path output_path;

	try {
		parse_commandline_options(argc, argv, input_paths, output_path);

		(void)setlocale(LC_ALL, "C");

		preprocess_input_list(input_paths);

		if(!output_path.mkdir()) {
			throw SysError("Failed to create directory " + output_path);
		}

		KBuild* bild;
		KAnimBankCollection banks;

		configure_bank_collection(banks);

		bild = process_input_list(input_paths, banks);

		if(bild == NULL) {
			throw KToolsError("No build found in the input files.");
		}
		if(banks.empty()) {
			throw KToolsError("No animation found in the input files.");
		}

		if(options::build_rename != nil) {
			if(options::verbosity >= 0) {
				cout << "Renaming build..." << endl;
			}
			bild->setName(options::build_rename);
		}

		if(options::verbosity >= 0) {
			cout << "Splicing build atlas..." << endl;
		}

		imglist_t imglist;
		imglist.reserve(bild->countFrames());
		bild->getImages( back_inserter(imglist) );

		if(options::verbosity >= 0) {
			cout << "Saving frame images..." << endl;
		}
		for(imglist_t::iterator it = imglist.begin(); it != imglist.end(); ++it) {
			Compat::Path imgoutpath = output_path/it->first;

			if(!imgoutpath.dirname().mkdir()) {
				throw SysError("Failed to create directory " + imgoutpath.dirname());
			}

			Magick::Image img = it->second;

			img.type(Magick::TrueColorMatteType);
			img.colorSpace(Magick::sRGBColorspace);

			// png color type 6 means RGBA
			img.defineValue("png", "color-type", "6");

			MAGICK_WRAP(img.write(imgoutpath));
		}

		if(options::verbosity >= 0) {
			cout << "Writing scml..." << endl;
		}
		Compat::Path scml_path = output_path/bild->getName() + ".scml";
		ofstream scml(scml_path.c_str());
		check_stream_validity(scml, scml_path);
		exportToSCML(scml, *bild, banks);

		if(options::verbosity >= 0) {
			cout << "Done." << endl;
		}
	}
	catch(std::exception& e) {
		cerr << "ERROR: " << e.what() << endl;
		return -1;
	}
	catch(...) {
		cerr << "ERROR" << endl;
		return -1;
	}
	
	return 0;
}
