#ifndef _LOG_H_INCLUDED
#	define _LOG_H_INCLUDED

typedef unsigned int verbosity_level_t;

enum verbosity_levels {
	LOG_ERROR   = 0,
	LOG_WARNING = 1,
	LOG_INFO    = 2,
	LOG_DEBUG   = 3
};

#	define DEFAULT_VERBOSITY_LEVEL (LOG_INFO)

void log_set_level(const verbosity_level_t level);
void log_write(const verbosity_level_t level, const char *format, ...);

#endif
