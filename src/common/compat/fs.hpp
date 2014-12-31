/*
 * Header-only library abstracting some filesystem properties.
 */

#ifndef KTOOLS_COMPAT_FS_HPP
#define KTOOLS_COMPAT_FS_HPP

#include "compat/common.hpp"
#include "compat/posix.hpp"

#include <string>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <cstdlib>
#include <cerrno>

#ifndef PATH_MAX
#	define PATH_MAX 65536
#endif

namespace Compat {
#ifdef IS_WINDOWS
#	define DIR_SEP_MACRO '\\'
#	define DIR_SEP_STR_MACRO "\\"
#else
#	define DIR_SEP_MACRO '/'
#	define DIR_SEP_STR_MACRO "/"
#endif

#if !defined(S_ISDIR)
#	error "The POSIX macro S_ISDIR is not defined."
#endif

	static const char NATIVE_DIRECTORY_SEPARATOR = DIR_SEP_MACRO;	

	static const char NATIVE_DUAL_DIRECTORY_SEPARATOR = (NATIVE_DIRECTORY_SEPARATOR == '/' ? '\\' : '/');

	template<char sep>
	class PathAbstraction : public std::string {
	public:
		static const char SEPARATOR = sep;
		static const char DUAL_SEPARATOR = (sep == '/' ? '\\' : '/');

		typedef struct stat stat_t;

		std::string& toString() {
			return static_cast<std::string&>(*this);
		}

		const std::string& toString() const {
			return static_cast<const std::string&>(*this);
		}

	private:
		void normalizeDirSeps(std::string& str) {
			if(SEPARATOR != '/') {
				std::string::iterator _begin = str.begin();
				std::replace(_begin, str.end(), DUAL_SEPARATOR, SEPARATOR);
			}
		}

		/*
		 * Skips extra slashes in a normalized string.
		 * Receives begin and end iterators, modifying them
		 * to the actual range.
		 */
		static void skip_extra_slashes(std::string::const_iterator& begin, std::string::const_iterator& end) {
			if(begin == end)
				return;

			const size_t len = static_cast<size_t>( std::distance(begin, end) );
			const std::string::const_iterator real_end = end;

			// It points to the last character, for now.
			end = begin;
			std::advance(end, len - 1);

			for(; begin != real_end && *begin == SEPARATOR; ++begin);
			for(; end != begin && *end == SEPARATOR; --end);
			++end;
		}


		int inner_stat(stat_t& buf) const {
			if(empty())
				return -1;
			return ::stat(this->c_str(), &buf);
		}


		size_t get_extension_position() const {
			const size_t dot_pos = find_last_of('.');
			const size_t last_sep_pos = find_last_of(SEPARATOR);
			if(dot_pos == npos || (last_sep_pos != npos && dot_pos < last_sep_pos)) {
				return npos;
			}
			else {
				return dot_pos + 1;
			}
		}
		
	public:
		bool stat(stat_t& buf) const {
			if(inner_stat(buf)) {
				buf = stat_t();
				return false;
			}
			return true;
		}

		stat_t stat() const {
			stat_t buf;
			stat(buf);
			return buf;
		}

		bool isDirectory() const {
#ifdef IS_WINDOWS
			DWORD dwAttrib = GetFileAttributes(c_str());
			return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
			stat_t buf;
			return stat(buf) && S_ISDIR(buf.st_mode);
#endif
		}


		/*
		 * Appends a string, preceded by a directory separator if necessary.
		 */
		PathAbstraction& appendPath(std::string str) {
			if(str.length() == 0)
				return *this;

			normalizeDirSeps(str);

			const size_t old_len = this->length();

			if(old_len != 0 || str[0] == SEPARATOR) {
				std::string::append(1, SEPARATOR);
			}
			std::string::const_iterator _begin, _end;
			_begin = str.begin();
			_end = str.end();
			skip_extra_slashes(_begin, _end);

			std::string::append(_begin, _end);
			return *this;
		}

		PathAbstraction& assignPath(const std::string& str) {
			std::string::clear();
			return appendPath(str);
		}

		PathAbstraction& operator=(const std::string& str) {
			return assignPath(str);
		}

		PathAbstraction& operator=(const char *str) {
			return assignPath(str);
		}

		PathAbstraction& operator=(const PathAbstraction& p) {
			std::string::assign(p);
			return *this;
		}

		bool makeAbsolute() {
			char resolved_path[PATH_MAX + 1];
			const char * status;
#ifdef IS_WINDOWS
			status = _fullpath(resolved_path, c_str(), sizeof(resolved_path) - 1);
#else
			status = realpath(c_str(), resolved_path);
#endif
			if( status != NULL ) {
				assignPath(resolved_path);
				return true;
			}
			else {
				return false;
			}
		}

		PathAbstraction(const std::string& str) : std::string() {
			*this = str;
		}

		PathAbstraction(const char* str) : std::string() {
			*this = str;
		}

		PathAbstraction(const PathAbstraction& p) : std::string() {
			*this = p;
		}

		PathAbstraction() {}

		virtual ~PathAbstraction() {}

		PathAbstraction copy() const {
			return PathAbstraction(*this);
		}

		PathAbstraction& operator+=(const std::string& str) {
			std::string::append(str);
			return *this;
		}

		PathAbstraction& operator+=(const char *str) {
			std::string::append(str);
			return *this;
		}

		PathAbstraction operator+(const std::string& str) const {
			return this->copy() += str;
		}

		PathAbstraction operator+(const char *str) const {
			return this->copy() += str;
		}

		PathAbstraction& operator/=(const std::string& str) {
			return appendPath(str);
		}

		PathAbstraction& operator/=(const char *str) {
			return appendPath(str);
		}

		PathAbstraction operator/(const std::string& str) const {
			return this->copy() /= str;
		}

		PathAbstraction operator/(const char *str) const {
			return this->copy() /= str;
		}

		/*
		 * Splits the path into directory and file parts.
		 *
		 * The "directory part" accounts for everything until the last
		 * separator (not including the separator itself). If no separator
		 * exists, the directory will be ".".
		 *
		 * The directory part DOES NOT include a trailing slash.
		 */
		void split(std::string& dir, std::string& base) const {
			assert( length() > 0 );

			const size_t last_sep = find_last_of(SEPARATOR);

			if(last_sep == 0 && length() == 1) {
				// Root directory (so Unix).
				dir = "/";
				base = "/";
				return;
			}

			if(last_sep == npos) {
				dir = ".";
				base = *this;
			}
			else {
				dir = substr(0, last_sep);
				base = substr(last_sep + 1);
			}
		}

		PathAbstraction dirname() const {
			PathAbstraction dir, base;
			split(dir, base);
			return dir;
		}

		// With trailing slash.
		PathAbstraction dirnameWithSlash() const {
			PathAbstraction p = dirname();
			if(p != "/") {
				p.append(1, SEPARATOR);
			}
			return p;
		}

		PathAbstraction basename() const {
			PathAbstraction dir, base;
			split(dir, base);
			return base;
		}

		std::string getExtension() const {
			const size_t start = get_extension_position();
			if(start == npos) {
				return "";
			}
			else {
				return substr(start);
			}
		}

		bool hasExtension(const std::string& ext) const {
			ssize_t len = ssize_t(length());
			ssize_t extlen = ssize_t(ext.length());

			ssize_t diff = len - extlen;

			if(diff <= 0 || (*this)[diff - 1] != '.') return false;

			for(ssize_t i = 0; i < extlen; i++) {
				if( tolower((*this)[diff + i]) != tolower(ext[i]) ) {
					return false;
				}
			}

			return true;
		}

		bool hasExtension() const {
			return get_extension_position() != npos;
		}

		void replaceExtension(const std::string& newext) {
			const size_t start = get_extension_position();
			if(start == npos) {
				std::string::append(1, '.');
			}
			else {
				std::string::erase(start);
			}
			std::string::append(newext);
		}

		PathAbstraction replaceExtension(const std::string& newext, bool) const {
			PathAbstraction ret(*this);
			ret.replaceExtension(newext);
			return ret;
		}

		PathAbstraction& removeExtension() {
			const size_t start = get_extension_position();
			if(start != npos) {
				assert( start >= 1 );
				std::string::erase(start - 1);
			}
			return *this;
		}

		PathAbstraction removeExtension() const {
			return PathAbstraction(*this).removeExtension();
		}

		bool exists() const {
			stat_t buf;
			return inner_stat(buf) == 0;
		}

		/*
		 * If p doesn't exist and *this does, returns true.
		 * Likewise, if *this doesn't exist and p does, returns false.
		 */
		bool isNewerThan(const PathAbstraction& p) const {
			stat_t mybuf, otherbuf;
			stat(mybuf);
			p.stat(otherbuf);

			if(mybuf.st_mtime > otherbuf.st_mtime) {
				return true;
			}
			else if(mybuf.st_mtime == otherbuf.st_mtime) {
				// Compares nanoseconds under Unix.
#if defined(IS_LINUX)
				return mybuf.st_mtim.tv_nsec > otherbuf.st_mtim.tv_nsec;
#elif defined(IS_MAC)
				return mybuf.st_mtimespec.tv_nsec > otherbuf.st_mtimespec.tv_nsec;
#endif
			}
			return false;
		}

		bool isOlderThan(const PathAbstraction& p) const {
			return p.isNewerThan(*this);
		}

		bool mkdir(mode_t mode = 0775, bool fail_on_existence = false) const {
			int status;
#if defined(IS_WINDOWS)
			(void)mode;
			status = ::_mkdir(c_str());
#else
			status = ::mkdir(c_str(), mode);
#endif
			if(status != 0) {
				if(!fail_on_existence && errno == EEXIST) {
					return isDirectory();
				}
				else {
					return false;
				}
			}
			
			return true;
		}
	};

	typedef PathAbstraction<NATIVE_DIRECTORY_SEPARATOR> Path;
	typedef PathAbstraction<'/'> UnixPath;
}

#endif
