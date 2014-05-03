#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "manager.h"

int main(int argc, char *argv[]) {
	/* the package mangager instance */
	manager_t manager = {{0}};

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* set the verbosity level to the maximum */
	//log_set_level(LOG_DEBUG);

	/* initialize the package manager */
	if (RESULT_OK != manager_new(&manager)) {
		goto end;
	}

	/* install or remove a package */
	if (0 == strcmp("fetch", argv[1])) {
		if (RESULT_OK != manager_fetch(&manager, argv[2], INSTALLATION_REASON_USER)) {
			goto close_package_manager;
		}
	} else {
		if (0 == strcmp("remove", argv[1])) {
			if (RESULT_OK != manager_remove(&manager, argv[2])) {
				goto close_package_manager;
			}
		} else
			goto close_package_manager;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

close_package_manager:
	/* shut down the package manager */
	manager_free(&manager);

end:
	return exit_code;
}