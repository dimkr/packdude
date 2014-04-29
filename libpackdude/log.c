#include <stdarg.h>
#include <stdio.h>
#include "log.h"

static unsigned int g_count = 0;

void log_write(const char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	if (0 <= vprintf(format, arguments)) {
		++g_count;

		if (LOG_CHUNK_SIZE == g_count) {
			(void) fflush(stdout);
			g_count = 0;
		}
	}
	va_end(arguments);
}
