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
	int strformat(std::string& s, const char *fmt, ...) {
		char buffer[4096];

		va_list ap;
		va_start(ap, fmt);

		const int ret = vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);

		va_end(ap);

		if(ret >= 0) {
			s.assign(buffer, ret);
		}

		return ret;
	}
}
