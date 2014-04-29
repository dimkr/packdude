#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../libpackdude/manager.h"
#include "../libpackdude/log.h"

int main(int argc, char *argv[]) {
	/* a package manager instance */
	manager_t manager;

	/* the installed package attributes */
	struct stat attributes;
	
	/* an error code */
	result_t result;

	/* initialize the package manager */
	result = manager_open(&manager, argv[3]);
	if (RESULT_OK != result)
		goto end;

	if (0 == strcmp("install", argv[1])) {
		if (-1 == stat(argv[1], &attributes)) {
			if (ENOENT != errno)
				goto end;
			result = manager_install_by_name(&manager, argv[2]);
		} else {
			result = manager_install_by_path(&manager, argv[2]);
		}
	} else {
		if (0 == strcmp("remove", argv[1]))
			result = manager_remove(&manager, argv[2], false);
		else
			result = RESULT_GENERIC_ERROR;
	}

	if (RESULT_ALREADY_INSTALLED == result)
		log_write("%s is already installed.\n", argv[2]);

	/* close the package manager */
	manager_close(&manager);

end:
	return (int) result;
}
