#ifndef KTOOLS_COMPAT_POSIX_HPP
#define KTOOLS_COMPAT_POSIX_HPP

/*
 * Windows includes many functions from POSIX, however it prepends an underscore.
 * This header provides a compatibility layer so they also work under Unix.
 */

#include "compat/common.hpp"

extern "C" {
#	include <sys/stat.h>
}

#ifdef IS_WINDOWS
#	include <direct.h>
#	include <windows.h>
#	include <tchar.h>
#	include <strsafe.h>
#	include <io.h>
#endif

#ifdef _MSC_VER
#	include <strsafe.h>
#endif

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
#if !defined(S_IFREG) && defined(_S_IFREG)
#	define S_IFREG _S_IFREG
#endif
#if !defined(S_ISDIR) && defined(_S_ISDIR)
#	define S_ISDIR _S_ISDIR
#endif
#if !defined(S_ISREG) && defined(_S_ISREG)
#	define S_ISREG _S_ISREG
#endif

#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
#	define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#	define S_ISDIR(mode) ((mode) & S_IFDIR)
#endif

#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#	define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
#	define S_ISREG(mode) ((mode) & S_IFREG)
#endif

#if !defined(HAVE_MODE_T)
typedef int mode_t;
#endif

#endif
