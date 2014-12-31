#ifndef KTOOLS_FILE_ABSTRACTION_HPP
#define KTOOLS_FILE_ABSTRACTION_HPP

#include "ktools_common.hpp"
#include <fstream>

namespace KTools {
	using std::ios_base;

	class VirtualPath;

	class VirtualDirectory : public Compat::Path {
		friend class VirtualPath;
	public:
		enum Type {
			REGULAR,
			ZIP,
			STANDARD_IO,
			XML_ATLAS,

			UNKNOWN
		};

		const Type type;


		typedef std::istream* (*in_handler_t)(const VirtualDirectory&, const Compat::Path& subpath, ios_base::openmode);
		typedef std::ostream* (*out_handler_t)(const VirtualDirectory&, const Compat::Path& subpath, ios_base::openmode);

		typedef std::pair<in_handler_t, out_handler_t> handler_pair_t;

		handler_pair_t getHandlers() const;

	private:
		VirtualDirectory(const Compat::Path& p, Type t) : Compat::Path(p), type(t) {}
		VirtualDirectory(const VirtualDirectory& d) : Compat::Path(d), type(d.type) {}

	public:
		virtual ~VirtualDirectory() {}

		Type getType() const {
			return type;
		}

		std::istream* open_in(const Compat::Path& subpath, ios_base::openmode mode = ios_base::openmode()) const {
			mode |= ios_base::in;
			return getHandlers().first(*this, subpath, mode);
		}

		std::ostream* open_out(const Compat::Path& subpath, ios_base::openmode mode = ios_base::openmode()) const {
			mode |= ios_base::out;
			return getHandlers().second(*this, subpath, mode);
		}
	};

	class VirtualPath : public Compat::Path {
		typedef Compat::Path super;

	public:
		typedef Compat::Path Path;

	private:
		VirtualDirectory getVirtualDirectory(Compat::Path& subpath) const;

		VirtualDirectory getVirtualDirectory() const {
			Compat::Path dummy;
			return getVirtualDirectory(dummy);
		}

#if defined(HAVE_LIBZIP)
		static bool zipEntryExists(const VirtualDirectory& zippath, const Compat::UnixPath& entrypath);
#endif

	public:
		VirtualPath(const char* str) : Path(str) {}
		VirtualPath(const std::string& str) : Path(str) {}
		VirtualPath(const Path& p) : Path(p) {}
		VirtualPath(const VirtualPath& p) : Path(p) {}
		VirtualPath() : Path() {}
		virtual ~VirtualPath() {}

		bool isZipEntry() const {
#if defined(HAVE_LIBZIP)
			return getVirtualDirectory().getType() == VirtualDirectory::ZIP;
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
			Compat::Path subpath;
			const VirtualDirectory& zippath = getVirtualDirectory(subpath);
			if(zippath.getType() == VirtualDirectory::ZIP) {
				return zipEntryExists(zippath, Compat::UnixPath(subpath));
			}
			else {
#endif
				return super::exists();
#if defined(HAVE_LIBZIP)
			}
#endif
		}

		// The pointer should later be deallocated by the user.
		std::istream* open_in(std::ios_base::openmode mode = std::ios_base::openmode()) const {
			Compat::Path subpath;
			VirtualDirectory d = getVirtualDirectory(subpath);
			return d.open_in(subpath, mode);
		}

		// The pointer should later be deallocated by the user.
		std::ostream* open_out(std::ios_base::openmode mode = std::ios_base::openmode()) const {
			Compat::Path subpath;
			VirtualDirectory d = getVirtualDirectory(subpath);
			return d.open_out(subpath, mode);
		}
	};
}

#endif
