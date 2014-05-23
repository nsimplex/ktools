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

#if !defined(IS_WINDOWS)
#	if defined(IS_LINUX) || defined(IS_MAC) || defined(BSD) || defined(unix) || defined(__unix__) || defined(__unix)
#		define IS_UNIX
#	endif
#endif

#if !defined(IS_UNIX) && !defined(IS_WINDOWS)
#	error "Unknown operating system"
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

#if defined(_MSC_VER)
#	if !defined(_CRT_SECURE_NO_WARNINGS)
#		define _CRT_SECURE_NO_WARNINGS 1
#	endif
// Deprecated funcs (such as snprintf).
#	pragma warning( disable: 4995 )
// Constant conditional expressions.
#	pragma warning( disable: 4127 )
// Failure to generate automatic assignment operator.
#	pragma warning( disable: 4512 )
#endif

#endif
