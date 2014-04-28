#include <stdlib.h>
#include "../libpackdude/package.h"

int main(int argc, char *argv[]) {
	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* a package */
	package_t package;

	/* open the package */
	if (false == package_open(argv[1], &package))
		goto end;

	/* install the package */
	if (false == package_install(&package, INSTALL_REASON_USER, argv[2]))
		goto close_package;

	/* report success */
	exit_code = EXIT_SUCCESS;

close_package:	
	/* close the package */
	package_close(&package);

end:
	return exit_code;
}
