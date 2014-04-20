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

#ifndef PATH_MAX
#	define PATH_MAX 65536
#endif

namespace Compat {
#ifdef IS_UNIX
#	define DIR_SEP_MACRO '/'
#	define DIR_SEP_STR_MACRO "/"
#else
#	define DIR_SEP_MACRO '\\'
#	define DIR_SEP_STR_MACRO "\\"
#endif

#if !defined(S_ISDIR)
#	error "The POSIX macro S_ISDIR is not defined."
#endif

	static const char DIRECTORY_SEPARATOR = DIR_SEP_MACRO;	

	static const char DUAL_DIRECTORY_SEPARATOR = (DIRECTORY_SEPARATOR == '/' ? '\\' : '/');

	class Path : public std::string {
	public:
		typedef struct stat stat_t;

		static const char SEPARATOR = DIRECTORY_SEPARATOR;

		std::string& toString() {
			return static_cast<std::string&>(*this);
		}

		const std::string& toString() const {
			return static_cast<const std::string&>(*this);
		}

	private:
		static void normalizeDirSeps(std::string& str, size_t offset = 0) {
			std::string::iterator _begin = str.begin();
			std::advance(_begin, offset);
			std::replace(_begin, str.end(), DUAL_DIRECTORY_SEPARATOR, DIRECTORY_SEPARATOR);
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

			for(; begin != real_end && *begin == DIRECTORY_SEPARATOR; ++begin);
			for(; end != begin && *end == DIRECTORY_SEPARATOR; --end);
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
			stat_t buf;
			return stat(buf) && S_ISDIR(buf.st_mode);
		}


		/*
		 * Appends a string, preceded by a directory separator if necessary.
		 */
		Path& appendPath(std::string str) {
			if(str.length() == 0)
				return *this;

			normalizeDirSeps(str);

			const size_t old_len = this->length();

			if(old_len != 0 || str[0] == DIRECTORY_SEPARATOR) {
				std::string::append(1, DIRECTORY_SEPARATOR);
			}
			std::string::const_iterator _begin, _end;
			_begin = str.begin();
			_end = str.end();
			skip_extra_slashes(_begin, _end);

			std::string::append(_begin, _end);
			return *this;
		}

		Path& assignPath(const std::string& str) {
			std::string::clear();
			return appendPath(str);
		}

		Path& operator=(const std::string& str) {
			return assignPath(str);
		}

		Path& operator=(const char *str) {
			return assignPath(str);
		}

		Path& operator=(const Path& p) {
			std::string::assign(p);
			return *this;
		}

		void makeAbsolute() {
			char resolved_path[PATH_MAX];
			if( realpath(c_str(), resolved_path) != NULL ) {
				assignPath(resolved_path);
			}
		}

		Path(const std::string& str) : std::string() {
			*this = str;
		}

		Path(const char* str) : std::string() {
			*this = str;
		}

		Path(const Path& p) : std::string() {
			*this = p;
		}

		Path() {}

		Path copy() const {
			return Path(*this);
		}

		Path& operator+=(const std::string& str) {
			std::string::append(str);
			return *this;
		}

		Path& operator+=(const char *str) {
			std::string::append(str);
			return *this;
		}

		Path operator+(const std::string& str) const {
			return this->copy() += str;
		}

		Path operator+(const char *str) const {
			return this->copy() += str;
		}

		Path& operator/=(const std::string& str) {
			return appendPath(str);
		}

		Path& operator/=(const char *str) {
			return appendPath(str);
		}

		Path operator/(const std::string& str) const {
			return this->copy() /= str;
		}

		Path operator/(const char *str) const {
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

			const size_t last_sep = find_last_of(DIRECTORY_SEPARATOR);

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

		Path dirname() const {
			Path dir, base;
			split(dir, base);
			return dir;
		}

		// With trailing slash.
		Path dirnameWithSlash() const {
			Path p = dirname();
			if(p != "/") {
				p.append(1, DIRECTORY_SEPARATOR);
			}
			return p;
		}

		Path basename() const {
			Path dir, base;
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

		Path& replaceExtension(const std::string& newext) {
			const size_t start = get_extension_position();
			if(start == npos) {
				std::string::append(1, '.');
			}
			else {
				std::string::erase(start);
			}
			std::string::append(newext);
			return *this;
		}

		Path& removeExtension() {
			const size_t start = get_extension_position();
			if(start != npos) {
				assert( start >= 1 );
				std::string::erase(start - 1);
			}
			return *this;
		}

		bool exists() const {
			stat_t buf;
			return inner_stat(buf) == 0;
		}

		operator bool() const {
			return exists();
		}

		/*
		 * If p doesn't exist and *this does, returns true.
		 * Likewise, if *this doesn't exist and p does, returns false.
		 */
		bool isNewerThan(const Path& p) const {
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
			else {
				return false;
			}
		}

		bool isOlderThan(const Path& p) const {
			return p.isNewerThan(*this);
		}
	};
}

#endif
