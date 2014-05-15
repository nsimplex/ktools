#include "kbuild.hpp"

using namespace std;

namespace Krane {
	const uint32_t KBuildFile::MAGIC_NUMBER = *reinterpret_cast<const uint32_t*>("BILD");

	const int32_t KBuildFile::MIN_VERSION = 5;
	const int32_t KBuildFile::MAX_VERSION = 6;

	bool KBuildFile::shouldHaveHashTable() const {
		return version >= 6;
	}

	void KBuildFile::loadFrom(const std::string& path, int verbosity) {
		if(verbosity >= 0) {
			std::cout << "Loading build from `" << path << "'..." << std::endl;
		}
		
		std::ifstream in(path.c_str(), std::ifstream::in | std::ifstream::binary);
		check_stream_validity(in, path);

		in.imbue(std::locale::classic());

		load(in, verbosity);
	}

	istream& KBuildFile::load(istream& in, int verbosity) {
		if(verbosity >= 1) {
			cout << "Loading build information..." << endl;
		}

		uint32_t magic;
		io.raw_read_integer(in, magic);
		if(!in || magic != MAGIC_NUMBER) {
			throw(KToolsError("Attempt to read a non-build file as build."));
		}

		io.raw_read_integer(in, version);

		if(version & 0xffff) {
			io.setNativeSource();
		}
		else {
			io.setInverseNativeSource();
			io.reorder(version);
		}

		if(verbosity >= 2) {
			cout << "Got build version " << version << "." << endl;
		}

		versionRequire();

		build = new KBuild();
		build->setParent(this);
		build->load(in, verbosity);

		return in;
	}

	istream& KBuild::load(istream& in, int verbosity) {
		typedef symbolmap_t::iterator symbol_iter;

		uint32_t numsymbols;
		io->read_integer(in, numsymbols);

		uint32_t numframes;
		io->read_integer(in, numframes);

		if(verbosity >= 1) {
			cout << "Reading build name..." << endl;
		}
		io->read_len_string<uint32_t>(in, name);
		if(verbosity >= 1) {
			cout << "\tGot build name: " << name << endl;
		}

		if(verbosity >= 1) {
			cout << "Reading atlas names..." << endl;
		}
		uint32_t numatlases;
		io->read_integer(in, numatlases);
		if(numatlases == 0) {
			throw(KToolsError("Build file has 0 atlases."));
		}
		atlases.resize(numatlases);
		for(uint32_t i = 0; i < numatlases; i++) {
			io->read_len_string<uint32_t>(in, atlases[i].first);
			if(verbosity >= 1) {
				cout << "\tGot atlas name: " << atlases[i].first << endl;
			}
		}

		if(verbosity >= 1) {
			cout << "Reading animation symbols..." << endl;
		}
		uint32_t effective_alphaverts = 0;
		for(uint32_t i = 0; i < numsymbols; i++) {
			hash_t h;
			io->read_integer(in, h);

			if(verbosity >= 4) {
				cout << "\tLoading symbol 0x" << hex << h << dec << "..." << endl;
			}

			Symbol& symb = symbols[h];
			symb.setParent(this);
			symb.setHash(h);
			if(!symb.loadPre(in, verbosity)) {
				throw(KToolsError("Failed to read animation symbol."));
			}

			effective_alphaverts += symb.countAlphaVerts();
		}

		if(!in || symbols.size() != numsymbols) {
			throw(KToolsError("Corrupted build file (hash collision)."));
		}

		uint32_t alphaverts;
		io->read_integer(in, alphaverts);
		if(alphaverts != effective_alphaverts) {
			throw(KToolsError("Corrupted build file (VB count mismatch)."));
		}

		if(verbosity >= 1) {
			cout << "Post-processing animation symbols..." << endl;
		}
		for(symbol_iter it = symbols.begin(); it != symbols.end(); ++it) {
			if(verbosity >= 4) {
				cout << "\tPost-processing symbol 0x" << hex << it->first << dec << "..." << endl;
			}
			if(!it->second.loadPost(in, verbosity)) {
				throw(KToolsError("Failed to post-process animation symbol."));
			}
		}

		if(!parent->shouldHaveHashTable() || !in || in.peek() == EOF) {
			if(parent->shouldHaveHashTable()) {
				std::cerr << "WARNING: Missing hash table at the end of the build file. Generating automatic names." << std::endl;
			}

			DataFormatter fmt;

			for(symbol_iter it = symbols.begin(); it != symbols.end(); ++it) {
				unsigned long id = (unsigned long)it->first;
				Symbol& symb = it->second;

				symb.name = fmt("symbol_%lx", id);
			}
		}
		else {
			if(verbosity >= 1) {
				cout << "Loading build hash table..." << endl;
			}
			uint32_t hashcollection_sz;
			io->read_integer(in, hashcollection_sz);

			uint32_t num_namedsymbols = 0;
			for(uint32_t i = 0; i < hashcollection_sz; i++) {
				hash_t h;
				io->read_integer(in, h);

				Symbol* symb = NULL;
				{
					symbol_iter it = symbols.find(h);
					if(it != symbols.end()) {
						symb = &(it->second);
					}
				}

				if(symb == NULL) {
					io->skip_len_string<uint32_t>(in);
				}
				else {
					io->read_len_string<uint32_t>(in, symb->name);
					num_namedsymbols++;

					if(strhash(symb->name) != symb->hash) {
						throw KToolsError("Hash for build symbol '" + symb->name + "' does not match its name.");
					}
				}

				if(verbosity >= 2) {
					cout << "\tGot 0x" << hex << h << dec << " => \"" << symb->name << "\"" << endl;
				}
			}
			if(num_namedsymbols != numsymbols) {
				if(num_namedsymbols < numsymbols) {
					throw(KToolsError("Incomplete build hash table (missing symbol name)."));
				}
				else {
					throw(logic_error("Uncaught hash collision."));
				}
			}
		}

		if(in.peek() != EOF) {
			std::cerr << "Warning: There is leftover data in the input build file." << std::endl;
		}

		return in;
	}

	istream& KBuild::Symbol::loadPre(istream& in, int verbosity) {
		if(!io) {
			throw(logic_error("Undefined BinIOHelper for animation symbol."));
		}

		uint32_t numframes;
		io->read_integer(in, numframes);
		frames.resize(numframes);

		if(verbosity >= 2) {
			cout << "\tReading symbol frames..." << endl;
		}
		for(uint32_t i = 0; i < numframes; i++) {
			if(verbosity >= 3) {
				cout << "\t\tReading symbol frame at position #" << (i + 1) << "..." << endl;
			}
			Frame& f = frames[i];
			f.setParent(this);
			if(!f.loadPre(in, verbosity)) {
				throw(KToolsError("Failed to load symbol frame."));
			}
		}
		update_framenumbermap();

		return in;
	}

	istream& KBuild::Symbol::Frame::loadPre(istream& in, int verbosity) {
		(void)verbosity;

		if(!io) {
			throw(logic_error("Undefined BinIOHelper for animation symbol frame."));
		}

		io->read_integer(in, framenum);
		io->read_integer(in, duration);

		float _x, _y, _w, _h;
		io->read_float(in, _x);
		io->read_float(in, _y);
		io->read_float(in, _w);
		io->read_float(in, _h);

		bbox.setDimensions(_x, _y, _w, _h);

		uint32_t alphaidx;
		io->read_integer(in, alphaidx);

		uint32_t alphacount;
		io->read_integer(in, alphacount);
		if(alphacount % 6 != 0) {
			throw(KToolsError("Corrupted build file (frame VB count should be a multiple of 6)."));
		}
		setAlphaVertCount(alphacount);

		return in;
	}

	istream& KBuild::Symbol::loadPost(istream& in, int verbosity) {
		const size_t numframes = frames.size();

		for(size_t i = 0; i < numframes; i++) {
			if(verbosity >= 3) {
				cout << "\t\tPost-processing symbol frame at position #" << (i + 1) << "..." << endl;
			}
			if(!frames[i].loadPost(in, verbosity)) {
				throw(KToolsError("Failed to post-process animation symbol frame."));
			}
		}

		return in;
	}

	istream& KBuild::Symbol::Frame::loadPost(istream& in, int verbosity) {
		(void)verbosity;

		Maybe<float> uvwdepth;

		uint32_t numtrigs = countTriangles();
		for(uint32_t i = 0; i < numtrigs; i++) {
			xyztriangle_type::vertex_type xyzs[3];
			uvwtriangle_type::vertex_type uvws[3];

			for(int j = 0; j < 3; j++) {
				float _x, _y, _z;
				io->read_float(in, _x);
				io->read_float(in, _y);
				io->read_float(in, _z);

				xyzs[j][0] = _x;
				xyzs[j][1] = _y;
				xyzs[j][2] = _z;

				float _u, _v, _w;
				io->read_float(in, _u);
				io->read_float(in, _v);
				io->read_float(in, _w);

				uvws[j][0] = _u;
				uvws[j][1] = _v;

				if(uvwdepth == nil) {
					uvwdepth = Just(_w);
				}
				else if( fabsf(uvwdepth - _w) >= 0.5f ) {
					throw KToolsError("Inconsistent uvw depth in build symbol frame.");
				}

				if(!in) {
					throw(KToolsError("Corrupt build file (failed to read VB vertex)."));
				}
			}

			xyztriangles[i].a = xyzs[0];
			xyztriangles[i].b = xyzs[1];
			xyztriangles[i].c = xyzs[2];

			uvwtriangles[i].a = uvws[0];
			uvwtriangles[i].b = uvws[1];
			uvwtriangles[i].c = uvws[2];
		}

		if(uvwdepth == nil) {
			atlas_depth = 0;
		}
		else {
			atlas_depth = int(floorf(uvwdepth + 0.5f));
		}

		updateAtlasBoundingBox();

		return in;
	}
}
