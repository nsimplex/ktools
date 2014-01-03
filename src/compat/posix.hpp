#ifndef KTECH_COMPAT_POSIX_HPP
#define KTECH_COMPAT_POSIX_HPP

/*
 * Windows includes many functions from POSIX, however it prepends an underscore.
 * This header provides a compatibility layer so they also work under Unix.
 */

extern "C" {
#	include <sys/stat.h>
}

#include <cstdio>

#ifdef _MSC_VER
#	ifndef stat
#		define stat _stat
#	endif
#	ifndef fstat
#		define fstat _fstat
#	endif
#	ifndef fileno
#		define fileno _fileno
#	endif
#else
#	ifndef _stat
#		define _stat stat
#	endif
#	ifndef _fstat
#		define _fstat fstat
#	endif
#	ifndef _fileno
#		define _fileno fileno
#	endif
#endif

#if !defined(S_IFMT) && defined(_S_IFMT)
#	define S_IFMT _S_IFMT
#endif
#if !defined(S_IFDIR) && defined(_S_IFDIR)
#	define S_IFDIR _S_IFDIR
#endif
#if !defined(S_ISDIR) && defined(_S_ISDIR)
#	define S_ISDIR _S_ISDIR
#endif
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
#	define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#endif
