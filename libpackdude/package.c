#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <zlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "package.h"

result_t package_open(const char *path, package_t *package) {
	/* the return value */
	result_t result = RESULT_FILE_MISSING;

	/* the package attributes */
	struct stat attributes;

	/* the raw package metadata */
	const char *raw_metadata;

	/* the archive contained inside the package */
	unsigned char *archive;

	/* the archive size */
	uInt archive_size;

	/* the package architecture */
	const char *architecture;

	/* get the package size */
	if (-1 == stat(path, &attributes))
		goto end;

	/* make sure the package size is at least the size of the header, 1 byte of metadata and a 1-byte archive */
	if ((sizeof(package_header_t) + 2) > attributes.st_size)
		goto end;

	/* open the package */
	package->fd = open(path, O_RDONLY);
	if (-1 == package->fd)
		goto end;

	/* map the archive contents to memory */
	package->contents = mmap(NULL, (size_t) attributes.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, package->fd, 0);
	if (NULL == package->contents) {
		result = RESULT_MMAP_FAILED;
		goto close_package;
	}

	/* locate the package header */
	package->header = (const package_header_t *) package->contents;

	/* locate the package metadata */
	raw_metadata = (const char *) package->contents + sizeof(package_header_t);

	/* make sure the metadata size is at least the size of "a=b", the smallest possible metadata */
	if ((3 * sizeof(char)) > package->header->metadata_size) {
		result = RESULT_INVALID_SIZE;
		goto unmap_contents;
	}

	/* locate the archive contained in the package */
 	archive = (unsigned char *) package->header + sizeof(package_header_t) + package->header->metadata_size;

	/* make sure the archive is not empty */
	archive_size = ((uInt) attributes.st_size) - (archive - package->contents);
	if (0 == archive_size) {
		result = RESULT_INVALID_SIZE;
		goto unmap_contents;
	}

	/* parse the package metadata */
	result = metadata_parse(raw_metadata, (size_t) package->header->metadata_size, &package->metadata);
	if (RESULT_OK != result)
		goto unmap_contents;

	/* get the package architecture */
	architecture = metadata_get(&package->metadata, "arch");
	if (NULL == architecture) {
		result = RESULT_NO_ARCHITECURE;
		goto unmap_contents;
	}

	/* check whether the package is incompatible */
	if ((0 != strcmp(ARCH, architecture)) && (0 != strcmp("all", architecture))) {
		result = RESULT_INCOMPATIBLE_ARCHITECURE;
		goto free_metadata;
	}

	/* verify the package checksum */
	if (((uInt) package->header->checksum) != crc32(crc32(0L, NULL, 0), archive, archive_size)) {
		result = RESULT_BAD_CHECKSUM;
		goto free_metadata;
	}

	/* open the archive */
	result = archive_open(archive, (size_t) archive_size, &package->archive);
	if (RESULT_OK != result)
		goto free_metadata;

	/* save the package size */
	package->size = attributes.st_size;

	/* report success */
	result = RESULT_OK;
	goto end;

free_metadata:
	/* free the package metadata */
	metadata_close(&package->metadata);

unmap_contents:
	/* unmap the package contents */
	(void) munmap(package->contents, (size_t) attributes.st_size);

close_package:
	/* close the package */
	(void) close(package->fd);

end:
	return result;
}

void package_close(package_t *package) {
	/* close the archive */
	archive_close(&package->archive);

	/* free the package metadata */
	metadata_close(&package->metadata);

	/* unmap the package contents */
	(void) munmap(package->contents, package->size);

	/* close the file */
	(void) close(package->fd);
}

result_t package_can_install(const package_t *package, const char *destination) {
	/* the return value */
	result_t result = RESULT_NO_DEPENDENCIES_FIELD;

	/* the package dependencies */
	char *dependencies;

	/* a single dependency */
	char *dependency;

	/* strtok_r()'s position within the dependencies list */
	char *position;

	/* get the package's dependencies list */
	dependencies = (char *) metadata_get(&package->metadata, "deps");
	if (NULL == dependencies)
		goto end;

	/* make sure all dependencies are installed */
	if (0 == strlen(dependencies))
		dependencies = NULL;
	else {
		dependencies = strdup(dependencies);
		if (NULL == dependencies) {
			result = RESULT_STRDUP_FAILED;
			goto end;
		}
		dependency = strtok_r(dependencies, " ", &position);
		if (NULL == dependency) {
			result = RESULT_INVALID_DEPENDENCIES;
			goto free_dependencies;
		}
		do {
			if (RESULT_NO == package_is_installed(dependency, destination))
				goto free_dependencies;
			dependency = strtok_r(NULL, " ", &position);
		} while (NULL != dependency);
	}

	/* report the package can be installed */
	result = RESULT_OK;

free_dependencies:
	/* free the dependencies list */
	if (NULL != dependencies)
		free(dependencies);

end:
	return result;
}

result_t package_install(package_t *package,
                     const install_reason_t reason,
                     const char *destination) {
	/* the return value */
	result_t result = RESULT_NO_NAME_FIELD;

	/* the package name */
	const char *name;

	/* the package metadata path */
	char metadata[PATH_MAX];

	/* get the package name */
	name = metadata_get(&package->metadata, "name");
	if (NULL == name)
		goto end;

	/* check whether the package can be installed */
	result = package_can_install(package, destination);
	if (RESULT_OK != result)
		goto end;

	/* add a metadata field which specifies the installation reason */
	result = metadata_add(&package->metadata, "reason", (const char *) reason);
	if (RESULT_OK != result)
		goto end;

	/* save a copy of the package metadata */
	(void) snprintf((char *) &metadata, sizeof(metadata), "%s/%s/%s", destination, PACKAGE_METADATA_DIR, name);
	result = metadata_dump(&package->metadata, (char *) &metadata);
	if (RESULT_OK != result)
		goto end;

	/* extract the archive */
	result = archive_extract(&package->archive, destination);
	if (RESULT_OK != result)
		goto end;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t package_is_installed(const char *name, const char *destination) {
	/* the return value */
	result_t is_installed = RESULT_NO;

        /* the package metadata path */
        char metadata[PATH_MAX];

	/* the metadata attributes */
	struct stat attributes;

	/* check whether the package metadata exists */
	(void) snprintf((char *) &metadata, sizeof(metadata), "%s/%s/%s", destination, PACKAGE_METADATA_DIR, name);
	if (-1 == stat((const char *) &metadata, &attributes))
		goto end;

	/* report the package is installed */
	is_installed = RESULT_YES;

end:
	return is_installed;
}
