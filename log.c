#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "log.h"

static verbosity_level_t min_verbosity_level = LOG_INFO;

static const char *g_verobosity_levels[] = {
	"ERROR",
	"WARNING",
	"INFO",
	"DEBUG"
};

void log_set_level(const verbosity_level_t level) {
	min_verbosity_level = level;
}

void log_write(const verbosity_level_t level, const char *format, ...) {
	/* the format string arguments */
	va_list arguments = {{0}};

	/* the current time, in textual form */
	char textual_time[26] = {'\0'};

	/* the current time, in binary form */
	time_t now = 0;

	/* string length */
	size_t length = 0;

	assert(NULL != format);
	assert((LOG_DEBUG == level) ||
	       (LOG_INFO == level) ||
	       (LOG_WARNING == level) ||
	       (LOG_ERROR == level));

	/* if the message has a low verbosity level, ignore it */
	if (min_verbosity_level < level) {
		return;
	}

	/* get the current time */
	(void) time(&now);

	/* convert the local time to a textual representation */
	if (NULL == ctime_r(&now, (char *) &textual_time)) {
		return;
	}

	/* strip the trailing line break */
	length = strlen((const char *) &textual_time) - 1;
	assert('\n' == textual_time[length]);
	textual_time[length] = '\0';

	/* print the time and the verbosity level */
	(void) printf("[%s](%s): ",
	              (const char *) &textual_time,
	              g_verobosity_levels[level]);

	va_start(arguments, format);
	(void) vprintf(format, arguments);
	va_end(arguments);
}
