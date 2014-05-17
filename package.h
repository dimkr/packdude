#ifndef _PACKAGE_H_INCLUDED
#	define _PACKAGE_H_INCLUDED

#	include <sys/types.h>
#	include <stdint.h>
#	include <arpa/inet.h>

#	include "result.h"
#	include "database.h"

/*!
 * @defgroup package Package
 * @brief Low-level package operations
 * @{ */

/*!
 * @def MAGIC
 * @brief The package magic
 * @see package_header_t */
#	define MAGIC (ntohl(0x65647564))

/*!
 * @struct package_header_t
 * @brief A package header
 * @see MAGIC */
typedef struct __attribute__((packed)) {
	uint32_t magic; /*!< A magic number */
	uint8_t version; /*!< The package format version */
	uint32_t checksum; /*!< A CRC32 checksum of the archive contained in the
	                    * package */
} package_header_t;

/*!
 * @struct package_t
 * @brief A package */
typedef struct {
	const char *path; /*!< The package path */
	int fd; /*!< The package file descriptor */
	unsigned char *contents; /*!< The package contents */
	size_t size; /*!< The package size */
	package_header_t *header; /*!< The package header */
	unsigned char *archive; /*!< The archive contained in the package */
	size_t archive_size; /*!< The archive size */
} package_t;

/*!
 * @struct file_register_params_t
 * @brief The parameters of _register_file() */
typedef struct {
	database_t *database; /*!< The database the file gets added to */
	const char *package; /*!< The package associated with the file */
} file_register_params_t;

/*!
 * @fn result_t result_t package_open(package_t *package, const char *path)
 * @brief Opens a package for reading
 * @param package The package
 * @param path The package path */
result_t package_open(package_t *package, const char *path);

/*!
 * @fn void package_close(package_t *package)
 * @brief Closes a package
 * @param package The package */
void package_close(package_t *package);

/*!
 * @fn result_t package_verify(const package_t *package)
 * @brief Verifies the integrity of a package
 * @param package The package */
result_t package_verify(const package_t *package);

/*!
 * @fn result_t package_install(const char *name,
 *                              package_t *package,
 *                              database_t *database);
 * @brief Installs a package
 * @param name The package name
 * @param package The package
 * @param database The database the package gets added to */
result_t package_install(const char *name,
                         package_t *package,
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