#ifndef _PACKAGE_H_INCLUDED
#	define _PACKAGE_H_INCLUDED

#	include <stdint.h>
#	include <arpa/inet.h>

#	include "result.h"

/*!
 * @defgroup package Package
 * @brief Package format handling
 * @{ */

/*!
 * @def MAGIC
 * @brief The package magic
 * @see package_header_t */
#	define MAGIC ((uint32_t) (ntohl(0x65647564)))

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
	unsigned char *contents; /*!< The package contents */
	size_t size; /*!< The package size */
	package_header_t *header; /*!< The package header */
	unsigned char *archive; /*!< The archive contained in the package */
	size_t archive_size; /*!< The archive size */
} package_t;

/*!
 * @fn result_t package_open(package_t *package,
 *                           unsigned char *contents,
 *                           const size_t size)
 * @brief Opens a package for reading
 * @param package The package
 * @param contents The package contents
 * @param size The package size */
result_t package_open(package_t *package,
                      unsigned char *contents,
                      const size_t size);

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
 * @} */

#endif
