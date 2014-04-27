#ifndef KTOOLS_COMPAT_COMMON_HPP
#define KTOOLS_COMPAT_COMMON_HPP

#include "config.h"

#if defined(_WIN32) || defined(WIN32)
#	define IS_WINDOWS 1
#	define OS_STRING "windows"
#elif defined(__linux__)
#	define IS_LINUX 1
#	define OS_STRING "linux"
#elif defined(__APPLE__) && defined(__MACH__)
#	define IS_MAC 1
#	define OS_STRING "osx"
#else
#	error "Unknown operating system"
#endif

#if defined(IS_LINUX) || defined(IS_MAC)
#	define IS_UNIX
#endif

#if !defined(HAVE_MODE_T)
typedef int mode_t
#endif

#ifdef __GNUC__
#	define PRINTFSTYLE(fmt_index, first_to_check) __attribute__ ((format (printf, fmt_index, first_to_check)))
#	define DEPRECATEDFUNCTION __attribute__ ((deprecated))
#	define CONSTFUNCTION __attribute__ ((const))
#	define PUREFUNCTION __attribute__ ((pure))
#	define HOTFUNCTION __attribute__ ((hot))
#else
#	define PRINTFSTYLE(fmt_index, first_to_check)
#	define DEPRECATEDFUNCTION
#	define CONSTFUNCTION
#	define PUREFUNCTION
#	define HOTFUNCTION
#endif

#endif
