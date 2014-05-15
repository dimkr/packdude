#ifndef _LOG_H_INCLUDED
#	define _LOG_H_INCLUDED

#	include <unistd.h>

/*!
 * @defgroup log Logging
 * @brief Logging and debugging information
 * @{ */

/*!
 * @typedef verbosity_level_t
 * @brief A message verbosity level */
typedef unsigned int verbosity_level_t;

enum verbosity_levels {
	LOG_NOTHING = 0,
	LOG_ERROR   = 1,
	LOG_WARNING = 2,
	LOG_INFO    = 3,
	LOG_DEBUG   = 4
};

/*!
 * @def DEFAULT_VERBOSITY_LEVEL
 * @brief The default verbosity level
 * @see log_set_level */
#	define DEFAULT_VERBOSITY_LEVEL (LOG_INFO)

/*!
 * @fn void log_set_level(const verbosity_level_t level)
 * @brief Sets the minimum message verbosity level
 * @param level The minimum verbosity level
 * @see DEFAULT_VERBOSITY_LEVEL
 *
 * Only messages with a verbosity level equal to or above a certain level are
 * printed. This function sets the minimum verbosity level. */
void log_set_level(const verbosity_level_t level);

/*!
 * @fn void log_write(const verbosity_level_t level, const char *format, ...)
 * @brief Outputs a formatted message, with additional information
 * @param level The message verbosity level
 * @param format The message \a printf() -style format string */
void log_write(const verbosity_level_t level, const char *format, ...);

/*!
 * @def log_dump
 * @brief Outputs a constant message */
#	define log_dump(x) (void) write(STDOUT_FILENO, x, sizeof(x) - sizeof(char))

/*!
 * @fn void log_dumpf(const char *format, ...)
 * @brief Outputs a formatted message, without any additional information
 * @param format The message \a printf() -style format string */
void log_dumpf(const char *format, ...);

/*!
 * @} */

#endif
