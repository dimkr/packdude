#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

#include "log.h"
#include "manager.h"

#define REPO_ENVIRONMENT_VARIABLE "REPO"

typedef unsigned int action_t;

enum actions {
	ACTION_INSTALL        = 0,
	ACTION_REMOVE         = 1,
	ACTION_LIST_INSTALLED = 2,
	ACTION_LIST_AVAILABLE = 3,
	ACTION_LIST_REMOVABLE = 4,
	ACTION_LIST_FILES     = 5,
	ACTION_INVALID        = 6
};

__attribute__((noreturn)) static void _show_help() {
	log_dump("Usage: packdude [-d] [-n] [-p PREFIX] [-u URL] -l|-q|-c|-f|-i|-r PACKAGE\n");
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

	/* a flag which indicates whether debugging output is enabled */
	bool debug = false;

	/* the package installation prefix */
	const char *prefix = DEFAULT_PREFIX;

	/* the package installation reason */
	const char *reason = INSTALLATION_REASON_USER;

	/* the installed or removed package */
	const char *package = NULL;

	/* the repository URL */
	const char *url = NULL;

	/* parse the command-line */
	do {
		option = getopt(argc, argv, "dnlqcf:u:i:r:p:");
		switch (option) {
			case 'd':
				debug = true;
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

			case 'n':
				reason = INSTALLATION_REASON_CORE;
				break;

			case 'q':
				action = ACTION_LIST_INSTALLED;
				verbosity_level = LOG_NOTHING;
				break;

			case 'l':
				action = ACTION_LIST_AVAILABLE;
				verbosity_level = LOG_NOTHING;
				break;

			case 'c':
				action = ACTION_LIST_REMOVABLE;
				verbosity_level = LOG_NOTHING;
				break;

			case 'f':
				action = ACTION_LIST_FILES;
				verbosity_level = LOG_NOTHING;
				package = optarg;
				break;

			case 'u':
				url = optarg;
				break;

			case (-1):
				switch (action) {
					case ACTION_REMOVE:
					case ACTION_LIST_FILES:
						if (NULL == package) {
							_show_help();
						}
						break;

					case ACTION_INSTALL:
						if (NULL == package) {
							_show_help();
						}

						/* fall-through */

					case ACTION_LIST_AVAILABLE:
						if (NULL == url) {
							url = getenv(REPO_ENVIRONMENT_VARIABLE);
							if (NULL == url) {
								_show_help();
							}
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
	if (true == debug) {
		verbosity_level = LOG_DEBUG;
	}
	log_set_level(verbosity_level);

	/* initialize the package manager */
	if (RESULT_OK != manager_new(&manager, prefix, url)) {
		goto end;
	}

	switch (action) {
		case ACTION_INSTALL:
			if (RESULT_OK != manager_fetch(&manager, package, reason)) {
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

		case ACTION_LIST_INSTALLED:
			if (RESULT_OK != manager_list_inst(&manager)) {
				goto close_package_manager;
			}
			break;

		case ACTION_LIST_AVAILABLE:
			if (RESULT_OK != manager_list_avail(&manager)) {
				goto close_package_manager;
			}
			break;

		case ACTION_LIST_REMOVABLE:
			if (RESULT_OK != manager_list_removable(&manager)) {
				goto close_package_manager;
			}
			break;

		case ACTION_LIST_FILES:
			if (RESULT_OK != manager_list_files(&manager, package)) {
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
