#ifndef KTOOLS_COMPAT_COMMON_HPP
#define KTOOLS_COMPAT_COMMON_HPP

#include "config.h"

#if defined(_WIN32) || defined(WIN32)
#	define IS_WINDOWS 1
#elif defined(__linux__)
#	define IS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#	define IS_MAC 1
#endif

#if defined(IS_LINUX) || defined(IS_MAC) || defined(BSD) || defined(unix) || defined(__unix__) || defined(__unix)
#	define IS_UNIX
#endif

#if !defined(IS_UNIX) && !defined(IS_WINDOWS)
#	error "Unknown operating system"
#endif

#if !defined(HAVE_MODE_T)
typedef int mode_t;
#endif

#ifdef __GNUC__
#	define PRINTFSTYLE(fmt_index, first_to_check) __attribute__ ((format (printf, fmt_index, first_to_check)))
#	define DEPRECATEDFUNCTION __attribute__ ((deprecated))
#	define CONSTFUNCTION __attribute__ ((const))
#	define PUREFUNCTION __attribute__ ((pure))
#	if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#		define HOTFUNCTION __attribute__ ((hot))
#	endif
#endif

#ifndef PRINTFSTYLE
#	define PRINTFSTYLE(fmt_index, first_to_check)
#endif
#ifndef DEPRECATEDFUNCTION
#	define DEPRECATEDFUNCTION
#endif
#ifndef CONSTFUNCTION
#	define CONSTFUNCTION
#endif
#ifndef PUREFUNCTION
#	define PUREFUNCTION
#endif
#ifndef HOTFUNCTION
#	define HOTFUNCTION
#endif

#ifdef IS_WINDOWS
#	ifndef _USE_MATH_DEFINES
#		define _USE_MATH_DEFINES 1
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX 1
#	endif
#endif

#endif
