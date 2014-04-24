/*
Copyright (C) 2013  simplex

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef KRANE_KANIM_HPP
#define KRANE_KANIM_HPP

#include "krane_common.hpp"
#include "binary_io_utils.hpp"
#include "algebra.hpp"

namespace Krane {
	class GenericKAnimFile;

	class KAnim;

	template<template<typename T, typename A> class Container = std::list, typename Alloc = std::allocator<KAnim*> >
	class KAnimFile;

	class KAnim : public NestedSerializer<GenericKAnimFile> {
		friend class GenericKAnimFile;

		template<template<typename T, typename A> class Container, typename Alloc>
		friend class KAnimFile;

	public:
		// For computations and such, the types defined by the binary data are inflexible.
		typedef computations_float_type float_type;

		static const uint8_t FACING_RIGHT = 1;
		static const uint8_t FACING_UP = 2;
		static const uint8_t FACING_LEFT = 4;
		static const uint8_t FACING_DOWN = 8;
		static const uint8_t FACING_SIDE = FACING_LEFT | FACING_RIGHT;
		static const uint8_t FACING_ANY = FACING_RIGHT | FACING_UP | FACING_LEFT | FACING_DOWN;

		class Frame : public NestedSerializer<KAnim> {
			friend class KAnim;

		public:
			typedef BoundingBox<float_type> bbox_type;

			class Event : public NestedSerializer<Frame> {
				friend class Frame;
				friend class KAnim;

				hash_t hash;
				std::string name;

			public:
				hash_t getHash() const {
					return hash;
				}
				const std::string& getName() const {
					return name;
				}

			private:
				/*
				std::istream& loadPre(std::istream& in, int verbosity);
				std::istream& loadPost(std::istream& in, const hashtable_t& ht, int verbosity);
				*/
			};

			class Element : public NestedSerializer<Frame> {
				friend class Frame;
				friend class KAnim;

			public:
				typedef ProjectiveMatrix<2, float_type> matrix_type;

			private:
				std::string name;
				hash_t hash;

				uint32_t build_frame;

				std::string layername;
				hash_t layername_hash;

				matrix_type M;
				float_type z;

			public:
				const std::string& getName() const {
					return name;
				}

				hash_t getHash() const {
					return hash;
				}

				uint32_t getBuildFrame() const {
					return build_frame;
				}

				const std::string& getLayerName() const {
					return layername;
				}

				const matrix_type& getMatrix() const {
					return M;
				}

				float_type getAnimSortOrder() const {
					return z;
				}

			private:
				std::istream& loadPre(std::istream& in, int verbosity);
				std::istream& loadPost(std::istream& in, const hashtable_t& ht, int verbosity);
			};

			typedef std::map<hash_t, Event> eventmap_t;
			eventmap_t events;

			typedef std::vector<Element> elementlist_t;
			elementlist_t elements;

			bbox_type bbox;

			float_type getDuration() const {
				return parent->getFrameDuration();
			}

			uint32_t countEvents() const {
				return uint32_t(events.size());
			}

			uint32_t countElements() const {
				return uint32_t(elements.size());
			}

		private:
			std::istream& loadPre(std::istream& in, int verbosity);
			std::istream& loadPost(std::istream& in, const hashtable_t& ht, int verbosity);
		};

	private:
		std::string name;

		std::string bank;
		hash_t bank_hash;

		uint8_t facing_byte;
		float_type frame_rate;

	public:
		typedef std::vector<Frame> framelist_t;
		framelist_t frames;

		const std::string& getName() const {
			return name;
		}

		// Includes the facing direction suffix, if any.
		void getFullName(std::string& fullname) const;

		std::string getFullName() const {
			std::string fullname;
			getFullName(fullname);
			return fullname;
		}

		void setName(const std::string& s) {
			// TODO: if this is ever really used, fix it to extract
			// the facing byte.
			name = s;
		}

		hash_t getBankHash() const {
			return bank_hash;
		}

		const std::string& getBank() const {
			return bank;
		}

		void setBank(const std::string& s) {
			bank = s;
		}

		uint32_t getFrameCount() const {
			return countFrames();
		}

		float_type getFrameRate() const {
			return frame_rate;
		}

		float_type getFrameDuration() const {
			return 1/frame_rate;
		}

		float_type getDuration() const {
			return getFrameCount()*getFrameDuration();
		}

		uint32_t countEvents() const {
			uint32_t n = 0;
			for(framelist_t::const_iterator it = frames.begin(); it != frames.end(); ++it) {
				n += it->countEvents();
			}
			return n;
		}

		uint32_t countFrames() const {
			return uint32_t(frames.size());
		}

		uint32_t countElements() const {
			uint32_t n = 0;
			for(framelist_t::const_iterator it = frames.begin(); it != frames.end(); ++it) {
				n += it->countElements();
			}
			return n;
		}

		virtual ~KAnim() {}

	private:
		std::istream& loadPre(std::istream& in, int verbosity);
		std::istream& loadPost(std::istream& in, const hashtable_t& ht, int verbosity);
	};


	/*
	 * Maps anim names to anims.
	 * Owns the pointers.
	 */
	class KAnimBank : public std::map<std::string, KAnim*>, NonCopyable {
		typedef std::map<std::string, KAnim*> super;

		hash_t hash;
		std::string name;

	public:
		hash_t getHash() const {
			return hash;
		}

		const std::string& getName() const {
			return name;
		}

		void setName(const std::string& _name) {
			name = _name;
			hash = strhash(_name);
		}

		void addAnim(KAnim* A) {
			if(A->getBankHash() != hash) {
				throw std::logic_error("Attempt to add an anim to the wrong bank.");
			}
			iterator match = find(A->getName());
			if(match != end()) {
				throw KToolsError("Duplicate anim '" + match->first + "' in bank '" + name + "'");
			}
			insert( std::make_pair(A->getName(), A) );
		}

		void clear() {
			for(iterator it = begin(); it != end(); ++it) {
				if(it->second != NULL) {
					delete it->second;
				}
			}
			super::clear();
		}

		KAnimBank() : super(), hash(), name() {}

		virtual ~KAnimBank() {
			clear();
		}
	};



	class GenericKAnimFile : public NonCopyable {
	public:
		static const uint32_t MAGIC_NUMBER;

		static const int32_t MIN_VERSION;
		static const int32_t MAX_VERSION;

		BinIOHelper io;

	private:
		int32_t version;

	public:
		static bool checkVersion(int32_t v) {
			return MIN_VERSION <= v && v <= MAX_VERSION;
		}

		bool checkVersion() const {
			return checkVersion(version);
		}

		void versionRequire() const {
			if(!checkVersion()) {
				std::cerr << "Animation file has unsupported encoding version " << version << "." << std::endl;
				throw(KToolsError("Animation has unsupported encoding version."));
			}
		}

		virtual uint32_t countEvents() const = 0;
		virtual uint32_t countFrames() const = 0;
		virtual uint32_t countElements() const = 0;

		virtual void setAnimCount(uint32_t n) = 0;

		void loadFrom(const std::string& path, int verbosity);
		std::istream& load(std::istream& in, int verbosity);

		virtual std::istream& loadPre_all_anims(std::istream& in, int verbosity) = 0;
		virtual std::istream& loadPost_all_anims(std::istream& in, const hashtable_t& ht, int verbosity) = 0;
	};


	template<template<typename T, typename A> class Container, typename Alloc>
	class KAnimFile : public GenericKAnimFile {
	public:
		typedef Container<KAnim*, Alloc> animcontainer_t;
		typedef typename animcontainer_t::iterator anim_iterator;
		typedef typename animcontainer_t::const_iterator anim_const_iterator;

		animcontainer_t anims;

		virtual uint32_t countEvents() const {
			uint32_t n = 0;
			for(anim_const_iterator it = anims.begin(); it != anims.end(); ++it) {
				n += (*it)->countEvents();
			}
			return n;
		}
		virtual uint32_t countFrames() const {
			uint32_t n = 0;
			for(anim_const_iterator it = anims.begin(); it != anims.end(); ++it) {
				n += (*it)->countFrames();
			}
			return n;
		}
		virtual uint32_t countElements() const {
			uint32_t n = 0;
			for(anim_const_iterator it = anims.begin(); it != anims.end(); ++it) {
				n += (*it)->countElements();
			}
			return n;
		}

		virtual void setAnimCount(uint32_t n) {
			clear();
			anims.resize(n);
		}


		virtual std::istream& loadPre_all_anims(std::istream& in, int verbosity) {
			for(anim_iterator it = anims.begin(); it != anims.end(); ++it) {
				*it = new KAnim;
				(*it)->setParent(this);
				if(!(*it)->loadPre(in, verbosity)) {
					throw(KToolsError("Failed to load animation."));
				}
			}
			return in;
		}

		virtual std::istream& loadPost_all_anims(std::istream& in, const hashtable_t& ht, int verbosity) {
			for(anim_iterator it = anims.begin(); it != anims.end(); ++it) {
				if(!(*it)->loadPost(in, ht, verbosity)) {
					throw(KToolsError("Failed to load animation."));
				}
			}
			return in;
		}


		void clear() {
			for(anim_iterator it = anims.begin(); it != anims.end(); ++it) {
				if(*it != NULL) {
					delete *it;
				}
			}
			anims.clear();
		}

		operator bool() const {
			return !anims.empty();
		}

		virtual ~KAnimFile() {
			clear();
		}
	};


	class KAnimSelector : public std::unary_function<const KAnim&, bool> {
	public:
		virtual bool select(const KAnim& A) {
			(void)A;
			return true;
		}

		bool operator()(const KAnim& A) {
			return select(A);
		}

		virtual ~KAnimSelector() {}
	};

	// Subclass which may be used on anims which just metadata
	// (i.e., before any frame was loaded)
	class KAnimMetadataSelector : public KAnimSelector {};

	class BankKAnimSelector : public KAnimSelector {
		std::set<hash_t> bank_set;

	public:
		void addBank(const std::string& name) {
			bank_set.insert( strhash(name) );
		}

		virtual bool select(const KAnim& A) {
			return bank_set.count(A.getBankHash()) > 0;
		}
	};

	/*
	// Owns the selector pointer.
	class KAnimFilter : public NonCopyable {
		KAnimSelector* s;

	public:
		void clear() {
			if(s != NULL) {
				delete s;
				s = NULL;
			}
		}

		void setSelector(KAnimSelector* _s) {
			clear();
			s = _s;
		}

		template<typename Container>
		void operator()(Container& C) {
			if(s == NULL) return;
			for(Container::iterator it = C.begin(); it != C.end();) {
				if((*s)(*it)) {
					++it;
				}
				else {
					C.erase(it++);
				}
			}
		}

		KAnimFilter() : s(NULL) {}
		~KAnimFilter() {
			clear();
		}
	};
	*/

	/*
	 * Maps a bank hash to a bank pointer.
	 * Owns the pointers.
	 * Owns the selector pointer.
	 */
	class KAnimBankCollection : public std::map<hash_t, KAnimBank*>, NonCopyable {
		typedef std::map<hash_t, KAnimBank*> super;

		KAnimSelector* s;

	public:
		void setSelector(KAnimSelector* _s) {
			clear();
			s = _s;
		}

		void clearBanks() {
			for(iterator it = begin(); it != end(); ++it) {
				if(it->second != NULL) {
					delete it->second;
				}
			}
		}

		void clearSelector() {
			if(s != NULL) {
				delete s;
				s = NULL;
			}
		}

		void clear() {
			clearBanks();
			clearSelector();
			super::clear();
		}

		// Now owns the anim.
		void addAnim(KAnim*& A) {
			if(!(s == NULL || (*s)(*A))) {
				delete A;
				A = NULL;
				return;
			}

			KAnimBank* B;

			iterator match = find(A->getBankHash());
			if(match == end()) {
				B = new KAnimBank;
				B->setName( A->getBank() );
				insert( std::make_pair(A->getBankHash(), B) );
			}
			else {
				B = match->second;
			}

			B->addAnim(A);
		}

		// Now owns all the anims in the range.
		template<typename Iterator>
		void addAnims(Iterator first, Iterator last) {
			while(first != last) {
				addAnim(*first++);
			}
		}

		KAnimBankCollection() : s(NULL) {}
	};
}

#endif
