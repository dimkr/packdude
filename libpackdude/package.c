#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <zlib.h>
#include <limits.h>
#include <stdio.h>
#include "package.h"

bool package_open(const char *path, package_t *package) {
	/* the return value */
	bool is_success = false;

	/* the package attributes */
	struct stat attributes;
	
	/* the raw package metadata */
	const char *raw_metadata;

	/* the archive contained inside the package */
	unsigned char *archive;
	
	/* the archive size */
	uInt archive_size;

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
	if (NULL == package->contents)
		goto close_package;

	/* locate the package header */
	package->header = (const package_header_t *) package->contents;

	/* locate the package metadata */
	raw_metadata = (const char *) package->contents + sizeof(package_header_t);

	/* make sure the metadata size is at least the size of "a=b", the smallest possible metadata */
	if ((3 * sizeof(char)) > package->header->metadata_size)
		goto unmap_contents;

	/* locate the archive contained in the package */
 	archive = (unsigned char *) package->header + sizeof(package_header_t) + package->header->metadata_size;

	/* make sure the archive is not empty */
	archive_size = ((uInt) attributes.st_size) - (archive - package->contents);
	if (0 == archive_size)
		goto unmap_contents;

	/* verify the package checksum */
	if (((uInt) package->header->checksum) != crc32(crc32(0L, NULL, 0), archive, archive_size))
		goto unmap_contents;

        /* parse the package metadata */
        if (false == metadata_parse(raw_metadata, package->header->metadata_size, &package->metadata))
                goto unmap_contents;

	/* open the archive */
	if (false == archive_open(archive, (size_t) archive_size, &package->archive))
		goto free_metadata;

	/* save the package size */
	package->size = attributes.st_size;

	/* report success */
	is_success = true;
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
	return is_success;
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

bool package_install(package_t *package, const install_reason_t reason, const char *destination) {
	/* the return value */
	bool is_success = false;

	/* the package name */
	const char *name;

	/* the package metadata path */
	char metadata[PATH_MAX];

	/* get the package name */
	name = metadata_get(&package->metadata, "name");
	if (NULL == name)
		goto end;

	/* add a metadata field which specifies the installation reason */
	if (false == metadata_add(&package->metadata, "reason", (const char *) reason))
		goto end;

	/* save a copy of the package metadata */
	(void) snprintf((char *) &metadata, sizeof(metadata), "%s/%s/%s", destination, PACKAGE_METADATA_DIR, name);
	if (false == metadata_dump(&package->metadata, (char *) &metadata))
		goto end;

	/* extract the archive */
	if (false == archive_extract(&package->archive, destination))
		goto end;

	/* report success */
	is_success = true;

end:
	return is_success;
}
