#include "ktools_common.hpp"

#include <stdarg.h>

#ifndef HAVE_SNPRINTF
int vsnprintf(char *str, size_t n, const char *fmt, va_list ap) {
	(void)n;
	return vsprintf(str, fmt, ap);
}

int snprintf(char *str, size_t n, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	const int ret = vsnprintf(str, n, fmt, ap);

	va_end(ap);

	return ret;
}
#endif

namespace KTools {
	static int vstrformat_inplace(std::string& s, const char *fmt, va_list ap) {
		char buffer[1 << 15];

		const int ret = vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);

		if(ret > 0) {
			s.assign(buffer, ret);
		}
		else {
			s.clear();
		}

		return ret;
	}

	int strformat_inplace(std::string& s, const char *fmt, ...) {
		va_list ap;
		va_start(ap, fmt);

		const int ret = vstrformat_inplace(s, fmt, ap);

		va_end(ap);

		return ret;
	}

	std::string strformat(const char *fmt, ...) {
		std::string s;

		va_list ap;
		va_start(ap, fmt);

		(void)vstrformat_inplace(s, fmt, ap);

		va_end(ap);

		return s;
	}

	const char * DataFormatter::operator()(const char * fmt, ...) const {
		va_list ap;
		va_start(ap, fmt);

		const int ret = vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);

		va_end(ap);

		if(ret >= 0) {
			return buffer;
		}
		else {
			throw Error("Failed to format string.");
			return NULL;
		}
	}



	void initialize_application(int& argc, char**& argv) {
		using namespace Compat;
		(void)argc;

#if defined(IS_WINDOWS) && defined(BUNDLED_DEPENDENCIES)
		static const char coderpath_varname[] = "MAGICK_CODER_MODULE_PATH";
		static const char filtpath_varname[] = "MAGICK_FILTER_MODULE_PATH";

		Path curdir = Path(argv[0]).dirname();

#	ifdef _MSC_VER
#		define SETENVVAR(name, value) _putenv_s(name, value)
#	else
#		define SETENVVAR(name, value) setenv(name, value, 1)
#	endif

		SETENVVAR(coderpath_varname, curdir.c_str());
		SETENVVAR(filtpath_varname, curdir.c_str());

#	undef SETENVVAR
#endif

		Magick::InitializeMagick(argv[0]);
	}
}
