#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "log.h"
#include "archive.h"
#include "package.h"

static result_t _print_path(const char *path, void *arg) {
	log_write(LOG_INFO, "Extracting %s\n", path);
	return RESULT_OK;
}

int main(int argc, char *argv[]) {
	/* the package attributes */
	struct stat attributes = {0};

	/* the package */
	package_t package = {0};

	/* the file descriptor */
	int fd = (-1);

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* the package contents */
	unsigned char *contents = NULL;

	/* make sure a package and an extraction destination were specified */
	if (3 != argc) {
		log_dump("Usage: dudeunpack PACKAGE DEST\n");
		goto end;
	}

	/* get the package size */
	if (-1 == stat(argv[1], &attributes)) {
		goto end;
	}

	/* open the package */
	fd = open(argv[1], O_RDONLY);
	if (-1 == fd) {
		goto end;
	}

	/* map the package contents to memory */
	contents = mmap(NULL,
	                (size_t) attributes.st_size,
	                PROT_READ,
	                MAP_PRIVATE,
	                fd,
	                0);
	if (NULL == contents) {
		goto close_file;
	}

	/* open the package */
	if (RESULT_OK != package_open(&package,
	                              contents,
	                              (size_t) attributes.st_size)) {
		goto unmap_contents;
	}

	/* verify the package integrity */
	if (RESULT_OK != package_verify(&package)) {
		goto close_package;
	}

	/* change the working directory to the extraction destination */
	if (-1 == chdir(argv[2])) {
		goto close_package;
	}

	/* extract the archive contained in the package */
	if (RESULT_OK != archive_extract(package.archive,
	                                 package.archive_size,
	                                 _print_path,
	                                 NULL)) {
		goto close_package;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

close_package:
	/* close the package */
	package_close(&package);

unmap_contents:
	/* unmap the package contents */
	(void) munmap(contents, (size_t) attributes.st_size);

close_file:
	/* close the file descriptor */
	(void) close(fd);

end:
	return exit_code;
}