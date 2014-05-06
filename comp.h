#ifndef _COMP_H_INCLUDED
#	define _COMP_H_INCLUDED

#	include <sys/types.h>

#	define MINIZ_HEADER_FILE_ONLY
#	include "miniz.c"

/*!
 * @defgroup comp Compression
 * @brief A compresion library wrapper
 * @{ */

/*!
 * @fn unsigned char *comp_compress(const unsigned char *data,
 *                                  const size_t size,
 *                                  size_t *compressed_size)
 * @brief Compresses a buffer
 * @param data The data to compress
 * @param size The data size
 * @param compressed_size The compressed data size */
unsigned char *comp_compress(const unsigned char *data,
                             const size_t size,
                             size_t *compressed_size);

/*!
 * @fn unsigned char *decomp_compress(const unsigned char *data,
 *                                    const size_t size,
 *                                    size_t *decompressed_size)
 * @brief Compresses a buffer
 * @param data The data to decompress
 * @param size The data size
 * @param decompressed_size The decompressed data size */
unsigned char *comp_decompress(const unsigned char *data,
                               const size_t size,
                               size_t *decompressed_size);

/*!
 * @} */

#endif