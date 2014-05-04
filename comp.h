#ifndef _COMP_H_INCLUDED
#	define _COMP_H_INCLUDED

#	include <sys/types.h>

#	define MINIZ_HEADER_FILE_ONLY
#	include "miniz.c"

unsigned char *comp_compress(const unsigned char *data,
                             const size_t size,
                             size_t *compressed_size);

unsigned char *comp_decompress(const unsigned char *data,
                               const size_t size,
                               size_t *decompressed_size);

#endif