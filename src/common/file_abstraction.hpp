#ifndef KTOOLS_FILE_ABSTRACTION_HPP
#define KTOOLS_FILE_ABSTRACTION_HPP

#include "ktools_common.hpp"
#include <fstream>

namespace KTools {
	class VirtualPath : public Compat::Path {
	public:
		typedef Compat::Path Path;

	private:
		std::istream* basic_openIn(std::ios_base::openmode mode) const {
			return new std::ifstream(c_str(), mode);
		}

		std::ostream* basic_openOut(std::ios_base::openmode mode = std::ios_base::out) const {
			return new std::ofstream(c_str(), mode);
		}

	public:
		VirtualPath(const std::string& str) : Path(str) {}
		VirtualPath(const Path& p) : Path(p) {}
		VirtualPath(const VirtualPath& p) : Path(p) {}
		VirtualPath() : Path() {}
		virtual ~VirtualPath() {}

		// The pointer should later be deallocated by the user.
		std::istream* openIn(std::ios_base::openmode mode = std::ios_base::openmode()) const {
			std::istream* ret = basic_openIn(mode | std::ios_base::in);
			check_stream_validity(*ret, *this);
			return ret;
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
