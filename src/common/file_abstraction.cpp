#include "file_abstraction.hpp"

using namespace std;

#if defined(HAVE_LIBZIP)
#	include <zip.h>
#endif

#if defined(HAVE_SSTREAM)
#	include <sstream>
typedef std::istringstream myistringstream;
#elif defined(HAVE_CLASS_STRSTREAM)
#	if defined(HAVE_STRSTREAM)
#		include <strstream>
#	else
#		include <strstream.h>
#	endif
typedef std::strstream myistringstream;
#else
#	error "No suitable string stream class found."
#endif

namespace KTools {
	// Owns the buffer.
	class bufferstream : public myistringstream {
		char *buf;

	public:
		bufferstream(char *_buf, size_t len) : buf(_buf) {
			rdbuf()->pubsetbuf(buf, len);
		}

		virtual ~bufferstream() {
			delete buf;
		}
	};

#if defined(HAVE_LIBZIP)
	size_t VirtualPath::getZipEntryPathPosition() const {
		// Includes the trailing slash.
		static const char zipext[] = {'.', 'z', 'i', 'p', SEPARATOR};
		static const size_t zipext_len = sizeof(zipext);

		size_t pos = find(zipext, 0, zipext_len);
		if(pos != npos && pos + zipext_len < length()) {
			return pos + zipext_len;
		}
		else {
			return npos;
		}
	}

	bool VirtualPath::zipEntryExists(size_t entry_pathpos) const {
		// Minus one because the last character is a slash.
		Compat::Path zippath = substr(0, ssize_t(entry_pathpos) - 1);
		Compat::UnixPath entrypath = substr(entry_pathpos);

		//cout << "exist check: " << entrypath << endl;

		struct zip * z = zip_open(zippath.c_str(), 0, NULL);
		if(z == NULL) {
			return false;
		}

		zip_int64_t idx = zip_name_locate(z, entrypath.c_str(), 0);

		zip_close(z);

		return idx >= 0;
	}

	std::istream* VirtualPath::zip_openIn(size_t entry_pathpos) const {
		Compat::Path zippath = substr(0, ssize_t(entry_pathpos) - 1);
		Compat::UnixPath entrypath = substr(entry_pathpos);

		int errorcode;
		struct zip * z = zip_open(zippath.c_str(), 0, &errorcode);
		if(z == NULL) {
			char errbuf[1024];
			zip_error_to_str(errbuf, sizeof(errbuf) - 1, errorcode, errno);
			throw KToolsError("Failed to open " + zippath + ": " + errbuf);
		}

		const zip_int64_t idx = zip_name_locate(z, entrypath.c_str(), 0);
		if(idx < 0) {
			zip_close(z);
			throw KToolsError("File " + entrypath + " does not exist in " + zippath);
		}

		struct zip_stat entry_info;

		if( zip_stat_index(z, idx, 0, &entry_info) != 0 ) {
			string zerr = zip_strerror(z);
			zip_close(z);
			throw KToolsError("Failed to stat " + entrypath + " in " + zippath + ": " + zerr);
		}

		struct zip_file* entry = zip_fopen_index( z, idx, ZIP_FL_UNCHANGED );
		if(entry == NULL) {
			string zerr = zip_strerror(z);
			zip_close(z);
			throw KToolsError("Failed to open " + entrypath + " in " + zippath + ": " + zerr);
		}

		char *buffer = new char[entry_info.size];

		const zip_int64_t read_cnt = zip_fread(entry, buffer, entry_info.size);

		if(read_cnt < 0) {
			string zerr = zip_file_strerror(entry);
			zip_fclose(entry);
			zip_close(z);
			throw KToolsError("Failed to read contents of " + entrypath + " in " + zippath + ": " + zerr);
		}

		zip_fclose(entry);
		zip_close(z);

		return new bufferstream(buffer, size_t(read_cnt));
	}
#endif
}
