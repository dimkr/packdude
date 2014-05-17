#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "log.h"
#include "archive.h"
#include "package_ops.h"

result_t _register_file(const char *path, file_register_params_t *params) {
	assert(NULL != path);
	assert(NULL != params);
	assert(NULL != params->database);
	assert(NULL != params->package);

	return database_register_path(params->database, path, params->package);
}

result_t package_install(const char *name,
                         package_t *package,
                         database_t *database) {
	/* the callback parameters */
	file_register_params_t params = {0};

	assert(NULL != name);
	assert(NULL != package);
	assert(NULL != database);

	log_write(LOG_INFO, "Unpacking %s\n", name);

	/* extract the archive */
	params.package = name;
	params.database = database;
	return archive_extract(package->archive,
	                       package->archive_size,
	                       (file_callback_t) _register_file,
	                       &params);
}

int _remove_file(database_t *database, int count, char **values, char **names) {
	/* the file attributes */
	struct stat attributes = {0};

	/* the return value */
	int abort = 0;

	assert(NULL != database);
	assert(NULL != values[FILE_FIELD_PATH]);

	log_write(LOG_DEBUG, "Removing %s\n", values[FILE_FIELD_PATH]);

	/* determine the file type - if it doesn't exist, it's fine */
	if (-1 == lstat(values[FILE_FIELD_PATH], &attributes)) {
		if (ENOENT != errno) {
			abort = 1;
		}
		goto end;
	}

	/* delete the file */
	if (S_ISDIR(attributes.st_mode)) {
		if (-1 == rmdir(values[FILE_FIELD_PATH])) {
			switch (errno) {
				case ENOTEMPTY:
				case EROFS:
					break;

				default:
					log_write(LOG_ERROR,
					          "Failed to remove %s\n",
					          values[FILE_FIELD_PATH]);
					abort = 1;
					goto end;
			}
		}
	} else {
		if (-1 == unlink(values[FILE_FIELD_PATH])) {
			log_write(LOG_ERROR,
			          "Failed to remove %s\n",
			          values[FILE_FIELD_PATH]);
			abort = 1;
			goto end;
		}
	}

	/* unregister the file */
	if (RESULT_OK != database_unregister_path(database,
	                                          values[FILE_FIELD_PATH])) {
		abort = 1;
	}

end:
	return abort;
}

result_t package_remove(const char *name, database_t *database) {
	/* the return value */
	result_t result = RESULT_OK;

	assert(NULL != name);
	assert(NULL != database);

	/* delete the package files */
	result = database_for_each_file(database,
	                                name,
	                                (query_callback_t) _remove_file,
	                                database);
	if (RESULT_OK != result) {
		goto end;
	}

	/* unregister the package */
	result = database_remove_installation_data(database, name);
	if (RESULT_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}
