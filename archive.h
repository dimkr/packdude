#ifndef _ARCHIVE_H_INCLUDED
#	define _ARCHIVE_H_INCLUDED

#	include <sys/types.h>

#	include "result.h"

/*!
 * @defgroup archive Archive
 * @brief Archive extraction
 * @{ */

/*!
 * @typedef file_callback_t
 * @brief A callback executed for each file extracted from an archive */
typedef result_t (*file_callback_t)(const char *path, void *arg);

/*!
 * @fn result_t archive_extract(unsigned char *contents,
 *                              const size_t size,
 *                              const file_callback_t callback,
 *                              void *arg)
 * @brief Extracts an archive
 * @param contents The archive
 * @param size The archive size
 * @param callback A callback to run for each extracted file
 * @param arg A pointer passed to the callback */
result_t archive_extract(unsigned char *contents,
                         const size_t size,
                         const file_callback_t callback,
                         void *arg);

/*!
 * @} */

#endif
