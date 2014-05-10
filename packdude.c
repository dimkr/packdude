#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "log.h"
#include "manager.h"

typedef unsigned int action_t;

enum actions {
	ACTION_INSTALL    = 0,
	ACTION_REMOVE     = 1,
	ACTION_LIST_INST  = 2,
	ACTION_LIST_AVAIL = 3,
	ACTION_INVALID    = 4
};

void _show_help() {
	log_dump("Usage: packdude [-d] [-p PREFIX] -l|-q|-i|-r PACKAGE\n");
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
	action_t action = ACTION_INVALID;

	/* the package installation prefix */
	const char *prefix = DEFAULT_PREFIX;

	/* the installed or removed package */
	const char *package = NULL;

	/* parse the command-line */
	do {
		option = getopt(argc, argv, "dlqi:r:p:");
		switch (option) {
			case 'd':
				verbosity_level = LOG_DEBUG;
				break;

			case 'p':
				prefix = optarg;
				break;

			case 'r':
				action = ACTION_REMOVE;
				package = optarg;
				break;

			case 'i':
				action = ACTION_INSTALL;
				package = optarg;
				break;

			case 'q':
				action = ACTION_LIST_INST;
				break;

			case 'l':
				action = ACTION_LIST_AVAIL;
				break;

			case (-1):
				switch (action) {
					case ACTION_INSTALL:
					case ACTION_REMOVE:
						if (NULL == package) {
							_show_help();
						}
						break;

					case ACTION_INVALID:
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
	switch (action) {
		case ACTION_INSTALL:
			if (RESULT_OK != manager_fetch(&manager,
			                               package,
			                               INSTALLATION_REASON_USER)) {
				goto close_package_manager;
			}
			break;

		case ACTION_REMOVE:
			if (RESULT_OK != manager_remove(&manager, package)) {
				goto close_package_manager;
			}

			/* once the package is removed, clean up all unneeded
			 * dependencies */
			if (RESULT_OK != manager_cleanup(&manager)) {
				goto close_package_manager;
			}

			break;

		case ACTION_LIST_INST:
			if (RESULT_OK != manager_list_inst(&manager)) {
				goto close_package_manager;
			}
			break;

		case ACTION_LIST_AVAIL:
			if (RESULT_OK != manager_list_avail(&manager)) {
				goto close_package_manager;
			}
			break;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

close_package_manager:
	/* shut down the package manager */
	manager_free(&manager);

end:
	return exit_code;
}