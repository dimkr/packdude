#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"
#include "comp.h"
#include "package.h"

int main(int argc, char *argv[]) {
	/* the archive attributes */
	struct stat attributes = {0};

	/* the package header */
	package_header_t header = {0};

	/* the compressed archive size */
	size_t compressed_size = 0;

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* the archive file descriptor */
	int fd = (-1);

	/* the output ifle */
	int output = (-1);

	/* the archive contents */
	unsigned char *archive = NULL;

	/* the compressed archive contents */
	unsigned char *compressed_archive = NULL;

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
	log_write(LOG_INFO, "Creating %s\n", argv[2]);
	output = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (-1 == output) {
		goto free_compressed_contents;
	}

	/* compress the archive */
	log_write(LOG_INFO, "Compressing %s\n", argv[1]);
	compressed_archive = comp_compress(archive,
	                                   (size_t) attributes.st_size,
	                                   &compressed_size);
	if (NULL == compressed_archive) {
		goto free_contents;
	}

	/* hash the archive */
	log_write(LOG_INFO, "Calculating the checksum of %s\n", argv[1]);
	header.checksum = (uint32_t) mz_crc32(MZ_CRC32_INIT,
	                                      compressed_archive,
	                                      compressed_size);

	/* write the package header */
	log_write(LOG_INFO, "Writing %s\n", argv[2]);
	header.version = VERSION;
	if (sizeof(header) != write(output, &header, sizeof(header))) {
		goto free_compressed_contents;
	}

	/* write the archive */
	if ((ssize_t) attributes.st_size != write(output,
	                                          compressed_archive,
	                                          compressed_size)) {
		goto free_compressed_contents;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

free_compressed_contents:
	/* free the compressed archive contents */
	free(compressed_archive);

free_contents:
	/* free the archive contents */
	free(archive);

close_archive:
	/* close the archive */
	(void) close(fd);

end:
	return exit_code;
}