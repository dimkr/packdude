#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

#include "log.h"
#include "manager.h"

void _show_help() {
	log_dump("Usage: packdude [-d] [-p PREFIX] -i|-r PACKAGE\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	/* the package mangager instance */
	manager_t manager = {{0}};

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* the verbosity level */
	verbosity_level_t verbosity_level = DEFAULT_VERBOSITY_LEVEL;

	/* a command-line option */
	int option = 0;

	/* the performed action */
	bool should_remove = false;

	/* the package installation prefix */
	const char *prefix = DEFAULT_PREFIX;

	/* the installed or removed package */
	const char *package = NULL;

	/* parse the command-line */
	do {
		option = getopt(argc, argv, "di:r:p:");
		switch (option) {
			case 'd':
				verbosity_level = LOG_DEBUG;
				break;

			case 'p':
				prefix = optarg;
				break;

			case 'r':
				should_remove = true;

			case 'i':
				if (NULL != package) {
					_show_help();
				};
				package = optarg;
				break;

			case (-1):
				if (NULL == package) {
					_show_help();
				}
				goto done;

			default:
				_show_help();
		}
	} while (1);

done:
	/* set the verbosity level */
	log_set_level(verbosity_level);

	/* initialize the package manager */
	if (RESULT_OK != manager_new(&manager, prefix)) {
		goto end;
	}

	/* install or remove a package */
	if (false == should_remove) {
		if (RESULT_OK != manager_fetch(&manager,
		                               package,
		                               INSTALLATION_REASON_USER)) {
			goto close_package_manager;
		}
	} else {
		if (RESULT_OK != manager_remove(&manager, package)) {
			goto close_package_manager;
		}

		/* once the package is removed, clean up all unneeded dependencies */
		if (RESULT_OK != manager_cleanup(&manager)) {
			goto close_package_manager;
		}
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

close_package_manager:
	/* shut down the package manager */
	manager_free(&manager);

end:
	return exit_code;
}