#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"
#include "comp.h"
#include "archive.h"
#include "package.h"

result_t _print_path(const char *path, void *arg) {
	log_write(LOG_INFO, "Extracting %s\n", path);
	return RESULT_OK;
}

int main(int argc, char *argv[]) {
	/* the archive attributes */
	struct stat attributes = {0};

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* the archive file descriptor */
	int fd = (-1);

	/* the package contents */
	unsigned char *contents = NULL;

	/* make sure a package and an extraction destination were specified */
	if (3 != argc) {
		log_dump("Usage: dudeunpack PACKAGE DEST\n");
		goto end;
	}

	/* verify the package integrity */
	if (RESULT_OK != package_verify(argv[1])) {
		goto end;
	}

	/* get the package size */
	log_write(LOG_DEBUG, "Getting the size of %s\n", argv[1]);
	if (-1 == stat(argv[1], &attributes)) {
		goto end;
	}

	/* open the package */
	log_write(LOG_INFO, "Reading %s\n", argv[1]);
	fd = open(argv[1], O_RDONLY);
	if (-1 == fd) {
		goto end;
	}

	/* change the working directory to the extraction destination */
	if (-1 == chdir(argv[2])) {
		goto close_package;
	}

	/* allocate memory for the package contents */
	contents = malloc((size_t) attributes.st_size);
	if (NULL == contents) {
		goto close_package;
	}

	/* read the package contents */
	if ((ssize_t) attributes.st_size != read(fd,
	                                         contents,
	                                         (size_t) attributes.st_size)) {
		goto free_contents;
	}

	/* extract the archive contained in the package */
	if (RESULT_OK != archive_extract(
		                 contents + sizeof(package_header_t),
	                     (size_t) attributes.st_size - sizeof(package_header_t),
	                     _print_path,
	                     NULL)) {
		goto free_contents;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

free_contents:
	/* free the package contents */
	free(contents);

close_package:
	/* close the package */
	(void) close(fd);

end:
	return exit_code;
}