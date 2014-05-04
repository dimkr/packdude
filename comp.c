#include "log.h"
#include "comp.h"

unsigned char *comp_compress(const unsigned char *data,
                             const size_t size,
                             size_t *compressed_size) {
	log_write(LOG_DEBUG, "Decompressing %zd bytes of data\n", size);
	return (unsigned char *) tdefl_compress_mem_to_heap(data,
	                                                    size,
	                                                    compressed_size,
	                                                    0);
}

unsigned char *comp_decompress(const unsigned char *data,
                               const size_t size,
                               size_t *decompressed_size) {
	log_write(LOG_DEBUG, "Compressing %zd bytes of data\n", size);
	return (unsigned char *) tinfl_decompress_mem_to_heap(data,
	                                                      size,
	                                                      decompressed_size,
	                                                      0);
}
