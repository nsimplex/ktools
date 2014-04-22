#include "krane.hpp"
#include "ktex/ktex.hpp"

using namespace Krane;
using namespace std;

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
		// TODO: catch magick warnings.
		Magick::Image img(path);
		return img;
	}
}

int main(int argc, char* argv[]) {
	Magick::InitializeMagick(argv[0]);

	if(argc != 3) {
		cerr << "Give me an input and an output dir as arguments!" << endl;
		exit(1);
	}

	Compat::Path inputdir = argv[1];
	assert(inputdir.isDirectory());

	Compat::Path outputdir = argv[2];
	assert(outputdir.mkdir());

	try {
		Compat::Path bildpath = inputdir/"build.bin";
		Compat::Path animpath = inputdir/"anim.bin";

		if(!bildpath.exists() || ! animpath.exists()) {
			throw(Error("no build.bin or no anim.bin"));
		}

		KBuildFile bildfile;
		bildfile.loadFrom(bildpath, 1);

		KAnimFile<> animfile;
		animfile.loadFrom(animpath, 1);

		KBuild& bild = *bildfile.getBuild();

		string atlasname = bild.getFrontAtlasName();
		Compat::Path atlaspath = inputdir/atlasname;

		bild.setFrontAtlas(load_image(atlaspath));

		typedef	list< pair<Compat::Path, Magick::Image> > imglist_t;
		imglist_t imglist;

		bild.getImages( back_inserter(imglist) );

		for(imglist_t::iterator it = imglist.begin(); it != imglist.end(); ++it) {
			cout << it->first << endl;

			Compat::Path outpath = outputdir/it->first;

			if(!outpath.dirname().mkdir()) {
				throw Error("failed to create dir.");
			}

			it->second.write(outpath);
		}

		cout << "Splitting banks..." << endl;

		KAnimBankCollection banks;
		banks.addAnims( animfile.anims.begin(), animfile.anims.end() );
		animfile.anims.clear();

		cout << "Writing scml..." << endl;
		ofstream scml((outputdir/bild.getName() + ".scml").c_str());
		if(!scml) {
			throw Error("failed to open output scml file.");
		}
		exportToSCML(scml, bild, banks);
	}
	catch(exception& e) {
		cerr << e.what() << endl;
		return -1;
	}
	
	return 0;
}
