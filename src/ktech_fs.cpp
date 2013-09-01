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


#include "ktech_fs.hpp"

extern "C" {
#ifdef HAVE_SYS_STAT_H
#	include <sys/stat.h>
#elif HAVE_STAT_H
#	include <stat.h>
#else
#	error A stat.h header is needed.
#endif

#ifdef HAVE_LIBGEN_H
#	include <libgen.h>
#else
#	error A libgen.h header is needed.
#endif
}

#include <cerrno>

using namespace KTech;
using namespace std;

bool KTech::isDirectory(const std::string& path) {
	struct stat st;

	if(stat(path.c_str(), &st) != 0) {
		throw Error(strerror(errno));
	}

	return S_ISDIR(st.st_mode);
}

void KTech::filepathSplit(const std::string& path, std::string& dir, std::string& notdir) {
	char *path_cp;
	
	path_cp = strdup( path.c_str() );
	dir = dirname(path_cp);
	free(path_cp);

	path_cp = strdup( path.c_str() );
	notdir = basename(path_cp);
	free(path_cp);

	if(notdir == "/") {
		notdir = "";
	}
}
