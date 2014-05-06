#ifndef _PACKAGE_H_INCLUDED
#	define _PACKAGE_H_INCLUDED

#	include <stdint.h>

#	include "result.h"
#	include "database.h"

/*!
 * @defgroup package Package
 * @brief Low-level package operations
 * @{ */

/*!
 * @struct package_header_t
 * @brief A package header */
typedef struct __attribute__((packed)) {
	uint8_t version; /*!< The package format version */
	uint32_t checksum; /*!< A CRC32 checksum of the archive contained in the package */
} package_header_t;

/*!
 * @struct file_register_params_t
 * @brief The parameters of _register_file() */
typedef struct {
	database_t *database; /*!< The database the file gets added to */
	const char *package; /*!< The package associated with the file */
} file_register_params_t;

/*!
 * @fn result_t package_verify(const char *path)
 * @brief Verifies the integrity of a package
 * @param path The package path */
result_t package_verify(const char *path);

/*!
 * @fn result_t package_install(const char *name,
 *                              const char *path,
 *                              database_t *database);
 * @brief Installs a package
 * @param name The package name
 * @param path The package path
 * @param database The database the package gets added to */
result_t package_install(const char *name,
                         const char *path,
                         database_t *database);

/*!
 * @fn result_t package_remove(const char *name, database_t *database);
 * @brief Removes a package
 * @param name The package name
 * @param databasee The database the package gets removed from */
result_t package_remove(const char *name, database_t *database);

/*!
 * @} */

#endif