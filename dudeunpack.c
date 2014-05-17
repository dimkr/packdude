#include <stdlib.h>

#include "log.h"
#include "comp.h"
#include "archive.h"
#include "package.h"

result_t _print_path(const char *path, void *arg) {
	log_write(LOG_INFO, "Extracting %s\n", path);
	return RESULT_OK;
}

int main(int argc, char *argv[]) {
	/* the package */
	package_t package = {0};

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* make sure a package and an extraction destination were specified */
	if (3 != argc) {
		log_dump("Usage: dudeunpack PACKAGE DEST\n");
		goto end;
	}

	/* open the package */
	if (RESULT_OK != package_open(&package, argv[1])) {
		goto end;
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

end:
	return exit_code;
}