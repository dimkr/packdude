#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

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

	/* get the archive size */
	if (-1 == stat(argv[1], &attributes)) {
		goto end;
	}

	/* open the archive */
	fd = open(argv[1], O_RDONLY);
	if (-1 == fd) {
		goto end;
	}

	/* allocate memory for the archive contents */
	archive = malloc((size_t) attributes.st_size);
	if (NULL == archive) {
		goto close_archive;
	}

	/* read the archive contents */
	if ((ssize_t) attributes.st_size != read(fd,
	                                         archive,
	                                         (size_t) attributes.st_size)) {
		goto free_contents;
	}

	/* open the output file */
	output = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (-1 == output) {
		goto free_contents;
	}

	/* hash the archive */
	header.checksum = (uint32_t) mz_crc32(MZ_CRC32_INIT,
	                                      archive,
	                                      (size_t) attributes.st_size);

	/* write the package header */
	header.version = 1;
	if (sizeof(header) != write(output, &header, sizeof(header))) {
		goto free_contents;
	}

	/* write the archive */
	if ((ssize_t) attributes.st_size != write(output,
	                                          archive,
	                                          (size_t) attributes.st_size)) {
		goto free_contents;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

free_contents:
	/* free the archive contents */
	free(archive);

close_archive:
	/* close the archive */
	(void) close(fd);

end:
	return exit_code;
}