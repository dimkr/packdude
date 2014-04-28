#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../libpackdude/manager.h"

int main(int argc, char *argv[]) {
	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* a package manager instance */
	manager_t manager;

	/* the installed package attributes */
	struct stat attributes;

	/* initialize the package manager */
	if (false == manager_open(&manager))
		goto end;

	if (0 == strcmp("install", argv[1])) {
		if (-1 == stat(argv[1], &attributes)) {
			if (ENOENT != errno)
				goto end;
			if (false == manager_install_by_name(&manager, argv[2], argv[3]))
				goto close_manager;
		} else {
			if (false == manager_install_by_path(&manager, argv[2], argv[3]))
				goto close_manager;
		}
	} else
		goto close_manager;

	/* report success */
	exit_code = EXIT_SUCCESS;

close_manager:
	/* close the package manager */
	manager_close(&manager);

end:
	return exit_code;
}
