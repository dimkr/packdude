#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <zlib.h>

#include "log.h"
#include "package.h"

int main(int argc, char *argv[]) {
	/* the archive attributes */
	struct stat attributes = {0};

	/* the package header */
	package_header_t header = {0};

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* the archive file descriptor */
	int fd = (-1);

	/* the output ifle */
	int output = (-1);

	/* the archive contents */
	unsigned char *archive = NULL;

	/* make sure an archive and an output file were specified */
	if (3 != argc) {
		log_dump("Usage: dudepack ARCHIVE PACKAGE\n");
		goto end;
	}

	/* get the archive size */
	log_write(LOG_DEBUG, "Getting the size of %s\n", argv[1]);
	if (-1 == stat(argv[1], &attributes)) {
		goto end;
	}

	/* open the archive */
	log_write(LOG_INFO, "Reading %s\n", argv[1]);
	fd = open(argv[1], O_RDONLY);
	if (-1 == fd) {
		goto end;
	}

	/* map the archive contents to memory */
	archive = mmap(NULL,
	               (size_t) attributes.st_size,
	               PROT_READ,
	               MAP_PRIVATE,
	               fd,
	               0);
	if (NULL == archive) {
		goto close_archive;
	}

	/* open the output file */
	log_write(LOG_INFO, "Creating %s\n", argv[2]);
	output = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (-1 == output) {
		goto unmap_archive;
	}

	/* hash the archive */
	log_write(LOG_INFO, "Calculating the checksum of %s\n", argv[1]);
	header.checksum = (uint32_t) crc32(crc32(0L, NULL, 0),
	                                   archive,
	                                   (uInt) attributes.st_size);

	/* write the package header */
	log_write(LOG_INFO, "Writing %s\n", argv[2]);
	header.version = VERSION;
	if (sizeof(header) != write(output, &header, sizeof(header))) {
		goto unmap_archive;
	}

	/* write the archive */
	if ((ssize_t) attributes.st_size != write(output,
	                                          archive,
	                                          (size_t) attributes.st_size)) {
		goto unmap_archive;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

unmap_archive:
	/* unmap the archive contents */
	(void) munmap(archive, (size_t) attributes.st_size);

close_archive:
	/* close the archive */
	(void) close(fd);

end:
	return exit_code;
}