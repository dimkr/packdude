#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

#include <zlib.h>

#include "log.h"
#include "archive.h"
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

	/* locate the package header */
	package->header = (package_header_t *) package->contents;

	/* locate the archive and calculate its size */
	package->archive = package->contents + sizeof(package_header_t);
	package->archive_size = package->size - sizeof(package_header_t);

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

	/* verify the package is targeted at the running package manager version */
	if (VERSION != package->header->version) {
		result = RESULT_INCOMPATIBLE;
		goto end;
	}

	/* verify the package checksum */
	if ((uLong) package->header->checksum != crc32(
	                                            crc32(0L, NULL, 0),
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


result_t _register_file(const char *path, file_register_params_t *params) {
	assert(NULL != path);
	assert(NULL != params);
	assert(NULL != params->database);
	assert(NULL != params->package);

	return database_register_path(params->database, path, params->package);
}

result_t package_install(const char *name,
                         package_t *package,
                         database_t *database) {
	/* the callback parameters */
	file_register_params_t params = {0};

	assert(NULL != name);
	assert(NULL != package);
	assert(NULL != database);

	log_write(LOG_INFO, "Unpacking %s\n", name);

	/* extract the archive */
	params.package = name;
	params.database = database;
	return archive_extract(package->archive,
	                       package->archive_size,
	                       (file_callback_t) _register_file,
	                       &params);
}

int _remove_file(database_t *database, int count, char **values, char **names) {
	/* the file attributes */
	struct stat attributes = {0};

	/* the return value */
	int abort = 0;

	assert(NULL != database);
	assert(NULL != values[FILE_FIELD_PATH]);

	log_write(LOG_DEBUG, "Removing %s\n", values[FILE_FIELD_PATH]);

	/* determine the file type - if it doesn't exist, it's fine */
	if (-1 == lstat(values[FILE_FIELD_PATH], &attributes)) {
		if (ENOENT != errno) {
			abort = 1;
		}
		goto end;
	}

	/* delete the file */
	if (S_ISDIR(attributes.st_mode)) {
		if (-1 == rmdir(values[FILE_FIELD_PATH])) {
			switch (errno) {
				case ENOTEMPTY:
				case EROFS:
					break;

				default:
					log_write(LOG_ERROR,
					          "Failed to remove %s\n",
					          values[FILE_FIELD_PATH]);
					abort = 1;
					goto end;
			}
		}
	} else {
		if (-1 == unlink(values[FILE_FIELD_PATH])) {
			log_write(LOG_ERROR,
			          "Failed to remove %s\n",
			          values[FILE_FIELD_PATH]);
			abort = 1;
			goto end;
		}
	}

	/* unregister the file */
	if (RESULT_OK != database_unregister_path(database,
	                                          values[FILE_FIELD_PATH])) {
		abort = 1;
	}

end:
	return abort;
}

result_t package_remove(const char *name, database_t *database) {
	/* the return value */
	result_t result = RESULT_OK;

	assert(NULL != name);
	assert(NULL != database);

	/* delete the package files */
	result = database_for_each_file(database,
	                                name,
	                                (query_callback_t) _remove_file,
	                                database);
	if (RESULT_OK != result) {
		goto end;
	}

	/* unregister the package */
	result = database_remove_installation_data(database, name);
	if (RESULT_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}
