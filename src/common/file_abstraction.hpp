#ifndef KTOOLS_FILE_ABSTRACTION_HPP
#define KTOOLS_FILE_ABSTRACTION_HPP

#include "ktools_common.hpp"
#include <fstream>

namespace KTools {
	class VirtualPath : public Compat::Path {
		typedef Compat::Path super;

	public:
		typedef Compat::Path Path;

	private:
		std::istream* basic_openIn(std::ios_base::openmode mode) const {
			return new std::ifstream(c_str(), mode);
		}

		std::ostream* basic_openOut(std::ios_base::openmode mode = std::ios_base::out) const {
			return new std::ofstream(c_str(), mode);
		}

#if defined(HAVE_LIBZIP)
		/*
		 * Returns the position of the start of the zip entry in the path, or
		 * string::npos if there is none.
		 */
		size_t getZipEntryPathPosition() const;

		bool zipEntryExists(size_t entry_pathpos) const;

		std::istream* zip_openIn(size_t entry_pathpos) const;
#endif

	public:
		VirtualPath(const std::string& str) : Path(str) {}
		VirtualPath(const Path& p) : Path(p) {}
		VirtualPath(const VirtualPath& p) : Path(p) {}
		VirtualPath() : Path() {}
		virtual ~VirtualPath() {}

		bool isZipEntry() const {
#if defined(HAVE_LIBZIP)
			return getZipEntryPathPosition() != npos;
#else
			return false;
#endif
		}

		bool isZipArchive() const {
#if defined(HAVE_LIBZIP)
			return hasExtension("zip") && super::exists();
#else
			return false;
#endif
		}

		bool exists() const {
#if defined(HAVE_LIBZIP)
			size_t entry_pathpos = getZipEntryPathPosition();
			if(entry_pathpos != npos) {
				return zipEntryExists(entry_pathpos);
			}
			else {
#endif
				return super::exists();
#if defined(HAVE_LIBZIP)
			}
#endif
		}

		// The pointer should later be deallocated by the user.
		std::istream* openIn(std::ios_base::openmode mode = std::ios_base::openmode()) const {
#if defined(HAVE_LIBZIP)
			size_t entry_pathpos = getZipEntryPathPosition();
			if(entry_pathpos != npos) {
				return zip_openIn(entry_pathpos);
			}
			else {
#endif
				std::istream* ret = basic_openIn(mode | std::ios_base::in);
				check_stream_validity(*ret, *this);
				return ret;
#if defined(HAVE_LIBZIP)
			}
#endif
		}

		// The pointer should later be deallocated by the user.
		std::ostream* openOut(std::ios_base::openmode mode = std::ios_base::openmode()) const {
			std::ostream* ret = basic_openOut(mode | std::ios_base::out);
			check_stream_validity(*ret, *this);
			return ret;
		}
	};
}

#endif
