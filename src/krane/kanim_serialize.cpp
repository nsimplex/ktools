#include "kanim.hpp"

using namespace std;

namespace Krane {
	const uint32_t GenericKAnimFile::MAGIC_NUMBER = *reinterpret_cast<const uint32_t*>("ANIM");

	const int32_t GenericKAnimFile::MIN_VERSION = 4;
	const int32_t GenericKAnimFile::MAX_VERSION = 4;


	void GenericKAnimFile::loadFrom(const string& path, int verbosity) {
		if(verbosity >= 0) {
			std::cout << "Loading animations from `" << path << "'..." << std::endl;
		}
		
		std::ifstream in(path.c_str(), std::ifstream::in | std::ifstream::binary);
		check_stream_validity(in, path);

		in.imbue(std::locale::classic());

		load(in, verbosity);
	}
	
	istream& GenericKAnimFile::load(istream& in, int verbosity) {
		if(verbosity >= 1) {
			cout << "Loading anim file information..." << endl;
		}

		uint32_t magic;
		io.raw_read_integer(in, magic);
		if(!in || magic != MAGIC_NUMBER) {
			throw(KToolsError("Attempt to read a non-anim file as anim."));
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
			cout << "Got anim version " << version << "." << endl;
		}

		versionRequire();


		uint32_t numelements;
		uint32_t numframes;
		uint32_t numevents;
		uint32_t numanims;

		io.read_integer(in, numelements);
		io.read_integer(in, numframes);
		io.read_integer(in, numevents);
		io.read_integer(in, numanims);

		setAnimCount(numanims);

		if(verbosity >= 1) {
			cout << "Loading " << numanims << " animations..." << endl;
		}

		if(!loadPre_all_anims(in, verbosity)) {
			throw(KToolsError("Failed to load animations."));
		}

		if(countElements() != numelements) {
			throw(KToolsError("Corrupt anim file (invalid element count)."));
		}
		if(countFrames() != numframes) {
			throw(KToolsError("Corrupt anim file (invalid frame count)."));
		}
		if(countEvents() != numevents) {
			throw(KToolsError("Corrupt anim file (invalid event count)."));
		}

		if(verbosity >= 1) {
			cout << "Loading anim hash table..." << endl;
		}

		uint32_t htsize;
		hashtable_t ht;
		io.read_integer(in, htsize);
		for(uint32_t i = 0; i < htsize; i++) {
			hash_t h;
			io.read_integer(in, h);

			string& str = ht[h];
			io.read_len_string<uint32_t>(in, str);

			if(verbosity >= 5) {
				cout << "\tGot 0x" << hex << h << dec << " => \"" << str << "\"" << endl;
			}
		}


		if(!loadPost_all_anims(in, ht, verbosity)) {
			throw(KToolsError("Failed to post-process animations."));
		}


		if(in.peek() != EOF) {
			std::cerr << "Warning: There is leftover data in the input anim file." << std::endl;
		}

		return in;
	}

	istream& KAnim::loadPre(istream& in, int verbosity) {
		io->read_len_string<uint32_t>(in, name);
		if(verbosity >= 1) {
			cout << "Got animation name: " << name << endl;
		}

		io->read_integer(in, facing_byte);
		io->read_integer(in, bank_hash);

		float _frame_rate;
		io->read_float(in, _frame_rate);
		frame_rate = _frame_rate;

		uint32_t numframes;
		io->read_integer(in, numframes);

		if(verbosity >= 1) {
			cout << "Loading animation frames..." << endl;
		}
		frames.resize(numframes);
		uint32_t framecounter = 0;
		for(framelist_t::iterator it = frames.begin(); it != frames.end(); ++it) {
			if(verbosity >= 2) {
				framecounter++;
				cout << "\tLoading animation frame " << framecounter << "..." << endl;
			}
			it->setParent(this);
			if(!it->loadPre(in, verbosity)) {
				throw(KToolsError("Failed to read animation frame."));
			}
		}

		return in;
	}

	istream& KAnim::loadPost(istream& in, const hashtable_t& ht, int verbosity) {
		hashtable_t::const_iterator match = ht.find(bank_hash);
		if(match == ht.end()) {
			throw(KToolsError("Incomplete anim hash table (missing bank name)."));
		}

		bank = match->second;
		if(verbosity >= 1) {
			cout << "Got bank name: " << bank << endl;
		}
		if(strhash(bank) != bank_hash) {
			throw KToolsError("Bank hash doesn't match bank name.");
		}

		if(verbosity >= 1) {
			cout << "Post-processing animation frames..." << endl;
		}
		uint32_t framecounter = 0;
		for(framelist_t::iterator it = frames.begin(); it != frames.end(); ++it) {
			if(verbosity >= 2) {
				framecounter++;
				cout << "\tPost-processing animation frame " << framecounter << "..." << endl;
			}
			if(!it->loadPost(in, ht, verbosity)) {
				throw(KToolsError("Failed to post-process animation frame."));
			}
		}

		return in;
	}

	istream& KAnim::Frame::loadPre(istream& in, int verbosity) {
		float _x, _y, _w, _h;
		io->read_float(in, _x);
		io->read_float(in, _y);
		io->read_float(in, _w);
		io->read_float(in, _h);

		bbox.setDimensions(_x, _y, _w, _h);

		uint32_t numevents;
		io->read_integer(in, numevents);

		if(verbosity >= 2) {
			cout << "\t\tLoading events..." << endl;
		}
		for(uint32_t i = 0; i < numevents; i++) {
			hash_t ev_hash;
			io->read_integer(in, ev_hash);

			if(verbosity >= 4) {
				cout << "\t\t\tGot event: 0x" << hex << ev_hash << dec << endl;
			}

			Event& ev = events[ev_hash];
			ev.setParent(this);
			ev.hash = ev_hash;
		}

		if(events.size() < numevents) {
			throw(KToolsError("Invalid anim file (hash collision in event names)."));
		}

		uint32_t numelements;
		io->read_integer(in, numelements);

		elements.resize(numelements);
		if(verbosity >= 2) {
			cout << "\t\tLoading elements..." << endl;
		}
		for(elementlist_t::iterator it = elements.begin(); it != elements.end(); ++it) {
			it->setParent(this);
			if(!it->loadPre(in, verbosity)) {
				throw(KToolsError("Failed to load animation element."));
			}
		}

		return in;
	}

	istream& KAnim::Frame::loadPost(istream& in, const hashtable_t& ht, int verbosity) {
		for(eventmap_t::iterator it = events.begin(); it != events.end(); ++it) {
			hashtable_t::const_iterator match = ht.find(it->first);

			if(match == ht.end()) {
				throw(KToolsError("Incomplete anim hash table (missing event name)."));
			}
			
			it->second.name = match->second;
		}

		if(verbosity >= 2) {
			cout << "\t\tPost-processing elements..." << endl;
		}
		for(elementlist_t::iterator it = elements.begin(); it != elements.end(); ++it) {
			it->loadPost(in, ht, verbosity);
		}

		return in;
	}

	istream& KAnim::Frame::Element::loadPre(istream& in, int verbosity) {
		io->read_integer(in, hash);

		if(verbosity >= 4) {
			cout << "\t\t\tLoading element 0x" << hex << hash << dec << "..." << endl;
		}

		io->read_integer(in, build_frame);

		io->read_integer(in, layername_hash);

		float m_a, m_b, m_c, m_d, m_tx, m_ty;
		io->read_float(in, m_a);
		io->read_float(in, m_b);
		io->read_float(in, m_c);
		io->read_float(in, m_d);
		io->read_float(in, m_tx);
		io->read_float(in, m_ty);

		M[0][0] = m_a;
		M[0][1] = m_b;
		M[1][0] = m_c;
		M[1][1] = m_d;

		M[0][2] = m_tx;
		M[1][2] = m_ty;

		float _z;
		io->read_float(in, _z);
		z = _z;

		return in;
	}

	istream& KAnim::Frame::Element::loadPost(istream& in, const hashtable_t& ht, int verbosity) {
		hashtable_t::const_iterator match;

		match = ht.find(hash);
		if(match == ht.end()) {
			throw(KToolsError("Incomplete anim hash table (missing element name)."));
		}

		name = match->second;
		if(verbosity >= 3) {
			cout << "\t\t\tGot element name: " << name << endl;
		}

		match = ht.find(layername_hash);
		if(match == ht.end()) {
			throw(KToolsError("Incomplete anim hash table (missing element layer name)."));
		}

		layername = match->second;

		return in;
	}
}
