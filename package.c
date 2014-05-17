#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <zlib.h>

#include "log.h"
#include "package.h"

result_t package_open(package_t *package, const char *path) {
	/* the package attributes */
	struct stat attributes = {0};

	/* the return value */
	result_t result = RESULT_IO_ERROR;

	assert(NULL != package);
	assert(NULL != path);

	log_write(LOG_DEBUG, "Opening %s\n", path);

	/* get the package size */
	if (-1 == stat(path, &attributes)) {
		goto end;
	}
	package->size = (size_t) attributes.st_size;

	/* make sure the package size is at bigger than the header size */
	if (sizeof(package_header_t) >= package->size) {
		log_write(LOG_ERROR, "%s is too small to be a valid package\n", path);
		result = RESULT_CORRUPT_DATA;
		goto end;
	}

	/* open the package */
	package->fd = open(path, O_RDONLY);
	if (-1 == package->fd) {
		goto end;
	}

	/* map the archive contents to memory */
	package->contents = mmap(NULL,
	                         package->size,
	                         PROT_READ,
	                         MAP_PRIVATE,
	                         package->fd,
	                         0);
	if (NULL == package->contents) {
		goto close_package;
	}

	/* locate the archive and calculate its size */
	package->archive = package->contents;
	package->archive_size = package->size - sizeof(package_header_t);

	/* locate the package header */
	package->header = (package_header_t *) (package->contents + \
	                                        package->size - \
	                                        sizeof(package_header_t));

	/* save the package path */
	package->path = path;

	/* report success */
	result = RESULT_OK;
	goto end;

close_package:
	/* close the package */
	(void) close(package->fd);

end:
	return result;
}

void package_close(package_t *package) {
	assert(NULL != package);
	assert(NULL != package->contents);

	/* free the package contents */
	log_write(LOG_DEBUG, "Closing %s\n", package->path);
	(void) munmap(package->contents, package->size);

	/* close the package */
	(void) close(package->fd);
}

result_t package_verify(const package_t *package) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != package);

	log_write(LOG_INFO, "Verifying the integrity of %s\n", package->path);

	/* verify the package is indeed a package, by checking the magic number */
	if (MAGIC != package->header->magic) {
		log_write(LOG_ERROR, "The package magic number is wrong\n");
		result = RESULT_CORRUPT_DATA;
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
		          "%s is corrupt; the checksum is incorrect\n",
		          package->path);
		result = RESULT_CORRUPT_DATA;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}
