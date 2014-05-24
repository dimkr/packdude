#include <stddef.h>
#include <assert.h>

#include <zlib.h>

#include "log.h"
#include "package.h"

result_t package_open(package_t *package,
                      unsigned char *contents,
                      const size_t size) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != package);
	assert(NULL != contents);
	assert(0 < size);

	/* make sure the package size is at bigger than the header size */
	if (sizeof(package_header_t) >= size) {
		log_write(LOG_ERROR, "The package is too small to be valid\n");
		goto end;
	}

	/* locate the archive and calculate its size */
	package->archive = contents;
	package->archive_size = size - sizeof(package_header_t);

	/* locate the package header */
	package->header = (package_header_t *) (contents + \
	                                        size - \
	                                        sizeof(package_header_t));

	/* save the package contains pointer and its size */
	package->contents = contents;
	package->size = size;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void package_close(package_t *package) {
	assert(NULL != package);
	assert(NULL != package->contents);
}

result_t package_verify(const package_t *package) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != package);

	log_write(LOG_INFO, "Verifying the package integrity\n");

	/* verify the package is indeed a package, by checking the magic number */
	if (MAGIC != package->header->magic) {
		log_write(LOG_ERROR, "The package magic number is wrong\n");
		goto end;
	}

	/* verify the package is targeted at the running package manager version */
	if (VERSION != package->header->version) {
		log_write(LOG_ERROR, "The package version is incompatible\n");
		result = RESULT_INCOMPATIBLE;
		goto end;
	}

	/* verify the package checksum */
	if ((uLong) package->header->checksum != crc32(
	                                            crc32(0L, Z_NULL, 0),
	                                            package->archive,
	                                            (uInt) package->archive_size)) {
		log_write(LOG_ERROR,
		          "The package is corrupt; the checksum is incorrect\n");
		result = RESULT_CORRUPT_DATA;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}
