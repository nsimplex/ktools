#include "kanim.hpp"

using namespace std;

namespace Krane {
	const uint32_t GenericKAnimFile::MAGIC_NUMBER = *reinterpret_cast<const uint32_t*>("ANIM");

	const int32_t GenericKAnimFile::MIN_VERSION = 3;
	const int32_t GenericKAnimFile::MAX_VERSION = 4;


	bool GenericKAnimFile::shouldHaveHashTable() const {
		return version >= 4;
	}


	class KAnimNamer {
	protected:
		KAnimNamer() {}
		KAnimNamer(const KAnimNamer&) {}
		virtual ~KAnimNamer() {}

	public:
		virtual std::string getBankName(hash_t) const = 0;
		virtual std::string getEventName(hash_t) const = 0;
		virtual std::string getElementName(hash_t) const = 0;
		virtual std::string getLayerName(hash_t) const = 0;
	};

	class HashTableKAnimNamer : public KAnimNamer {
		hashtable_t* ht;

		std::string doFail(const char *msg) const {
			throw KToolsError(msg);
			return "";
		}

		std::string resolveHash(hash_t h, const char *failmsg) const {
			hashtable_t::const_iterator match = ht->find(h);
			if(match == ht->end()) {
				return doFail(failmsg);
			}
			else {
				return match->second;
			}
		}

	public:
		HashTableKAnimNamer(hashtable_t& _ht) : ht(&_ht) {}

		virtual std::string getBankName(hash_t h) const {
			return resolveHash(h, "Incomplete anim hash table (missing bank name).");
		}
		virtual std::string getEventName(hash_t h) const {
			return resolveHash(h, "Incomplete anim hash table (missing event name).");
		}
		virtual std::string getElementName(hash_t h) const {
			return resolveHash(h, "Incomplete anim hash table (missing element name).");
		}
		virtual std::string getLayerName(hash_t h) const {
			return resolveHash(h, "Incomplete anim hash table (missing layer name).");
		}
	};

	class FallbackKAnimNamer : public KAnimNamer {
		mutable DataFormatter fmt;

		/*
		mutable hashtable_t eventcache;
		mutable hashtable_t elementcache;
		mutable hashtable_t layercache;

		std::string resolveHash(hash_t h, hashtable_t& cache, const char* prefix) const {
			std::string* ret;

			hashtable_t::iterator match = cache.find(h);
			if(match == cache.end()) {
				unsigned long id = (unsigned long)cache.size() + 1;
				std::string& cached_res = cache[h];
				cached_res.assign(fmt("%s%02lu", prefix, id));
				ret = &cached_res;
			}
			else {
				ret = &(match->second);
			}
			return *ret;
		}
		*/

		std::string resolveHash(hash_t h, const char *prefix) const {
			unsigned long id = (unsigned long)h;
			return fmt("%s%02lx", prefix, id);
		}

	public:
		FallbackKAnimNamer() {}

		virtual std::string getBankName(hash_t h) const {
			return resolveHash(h, "bank_");
		}
		virtual std::string getEventName(hash_t h) const {
			return resolveHash(h, "event_");
		}
		virtual std::string getElementName(hash_t h) const {
			return resolveHash(h, "element_");
		}
		virtual std::string getLayerName(hash_t h) const {
			return resolveHash(h, "layer_");
		}
	};


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


		if(!shouldHaveHashTable() || !in || in.peek() == EOF) {
			if(shouldHaveHashTable()) {
				std::cerr << "WARNING: Missing hash table at the end of the anim file. Generating automatic names." << std::endl;
			}

			FallbackKAnimNamer namer;

			(void)loadPost_all_anims(in, namer, verbosity);
		}
		else {
			if(verbosity >= 1) {
				cout << "Loading anim hash table..." << endl;
			}

			hashtable_t ht;
			uint32_t htsize;
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

			HashTableKAnimNamer namer(ht);

			(void)loadPost_all_anims(in, namer, verbosity);
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

	istream& KAnim::loadPost(istream& in, const KAnimNamer& ht, int verbosity) {
		bank = ht.getBankName(bank_hash);
		if(verbosity >= 1) {
			cout << "Got bank name: " << bank << endl;
		}
		/*
		if(strhash(bank) != bank_hash) {
			throw KToolsError("Bank hash doesn't match bank name.");
		}
		*/

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

	istream& KAnim::Frame::loadPost(istream& in, const KAnimNamer& ht, int verbosity) {
		for(eventmap_t::iterator it = events.begin(); it != events.end(); ++it) {
			it->second.name = ht.getEventName(it->first);
		}

		if(verbosity >= 2) {
			cout << "\t\tPost-processing elements..." << endl;
		}
		for(elementlist_t::iterator it = elements.begin(); it != elements.end(); ++it) {
			it->loadPost(in, ht, verbosity);
		}

		/*
		if(1) {
			typedef std::set< std::pair<hash_t, float_type> > myset_t;
			std::map<hash_t, myset_t > elem_layer_set;
			for(elementlist_t::iterator it = elements.begin(); it != elements.end(); ++it) {
				myset_t& myset = elem_layer_set[it->getHash()];
				myset_t::value_type val = std::make_pair( it->layername_hash, it->z );
				if(myset.count(val)) {
					std::cerr << ("Same element with duplicate layers: " + it->getName() + ", " + it->layername) << std::endl;
				}
				myset.insert(val);
			}
		}
		*/

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

	istream& KAnim::Frame::Element::loadPost(istream& in, const KAnimNamer& ht, int verbosity) {
		name = ht.getElementName(hash);
		if(verbosity >= 3) {
			cout << "\t\t\tGot element name: " << name << endl;
		}

		layername = ht.getLayerName(layername_hash);

		return in;
	}
}
