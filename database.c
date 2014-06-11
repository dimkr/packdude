#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sqlite3.h>

#include "log.h"
#include "database.h"

static const char *g_initialization_queries[] = {
	METADATA_DATABASE_CREATION_QUERY,
	INSTALLATION_DATA_DATABASE_CREATION_QUERY
};

void package_info_free(package_info_t *info) {
	/* a loop index */
	int i = INSTALLATION_DATA_FIELDS_COUNT - 1;

	assert(NULL != info);

	for ( ; 0 <= i; --i) {
		if (NULL != info->_fields[i]) {
			free(info->_fields[i]);
		}
	}
}

static result_t _open_database(database_t *database,
                               const char *path,
                               const int flags) {
	/* the return value */
	result_t result = RESULT_DATABASE_ERROR;

	assert(NULL != database);
	assert(NULL != path);

	/* open the database for reading */
	if (SQLITE_OK != sqlite3_open_v2(path,
	                                 &database->handle,
	                                 flags,
	                                 NULL)) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_open_read(database_t *database, const char *path) {
	assert(NULL != database);
	assert(NULL != path);

	log_write(LOG_DEBUG, "Opening %s for reading\n", path);
	return _open_database(database, path, SQLITE_OPEN_READONLY);
}

static result_t _run_query(database_t *database,
                           const char *query,
                           const query_callback_t callback,
                           void *arg) {
	/* the return value */
	result_t result = RESULT_DATABASE_ERROR;

	/* an error message */
	char *error = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != query);

	/* run the query */
	log_write(LOG_DEBUG, "Running a SQL query: %s\n", query);
	switch (sqlite3_exec(database->handle, query, callback, arg, &error)) {
		case SQLITE_OK:
			result = RESULT_OK;
			break;

		case SQLITE_ABORT:
			log_write(LOG_DEBUG, "The SQL query has been aborted\n");
			result = RESULT_ABORTED;
			break;

		default:
			assert(NULL != error);
			log_write(LOG_DEBUG, "An SQLite3 error occurred: %s\n", error);
	}

	if (NULL != error) {
		sqlite3_free(error);
	}

	return result;
}

result_t database_open_write(database_t *database,
                             const database_type_t type,
                             const char *path) {
	/* the database attributes */
	struct stat attributes = {0};

	/* the return value */
	result_t result = RESULT_DATABASE_ERROR;

	/* a flag which indicates whether the database exists */
	bool exists = true;

	assert(NULL != database);
	assert((DATABASE_TYPE_METADATA == type) ||
	       (DATABASE_TYPE_INSTALLATION_DATA == type));
	assert(NULL != path);

	/* check whether the database exists - if no, initialize it */
	if (-1 == stat(path, &attributes)) {
		if (ENOENT != errno) {
			goto end;
		}
		exists = false;
	}

	/* create the database and open it for writing */
	log_write(LOG_DEBUG, "Opening %s for writing\n", path);
	result = _open_database(database,
	                        path,
	                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
	if (RESULT_OK != result) {
		goto end;
	}

	/* if the database should be initialized, create empty tables */
	if (false == exists) {
		result = _run_query(database,
		                    g_initialization_queries[(unsigned int) type],
		                    NULL,
		                    NULL);
		if (RESULT_OK != result) {
			database_close(database);
			goto end;
		}
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void database_close(database_t *database) {
	assert(NULL != database);

	/* close the database */
	(void) sqlite3_close(database->handle);
}

static int _copy_info(void *arg, int count, char **values, char **names) {
	/* a loop index */
	int i = 0;

	assert(NULL != arg);
	assert((METADATA_FIELDS_COUNT == count) ||
	       (INSTALLATION_DATA_FIELDS_COUNT == count));
	assert(NULL != values);
	assert(NULL != names);

	/* copy all fields except those used internally */
	for ( ; (count - PRIVATE_FIELDS_COUNT) > i; ++i) {
		((package_info_t *) arg)->_fields[i] = strdup(values[i]);
		if (NULL == ((package_info_t *) arg)->_fields[i]) {
			for ( ; 0 <= i; --i) {
				free(((package_info_t *) arg)->_fields[i]);
			}
			return 1;
		}
	}

	return 0;
}

result_t database_get(database_t *database,
                      const char *name,
                      package_info_t *info) {
	/* the return value */
	result_t result = RESULT_DATABASE_ERROR;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != name);
	assert(NULL != info);

	log_write(LOG_DEBUG, "Searching the package database for %s\n", name);

	/* format the query */
	query = sqlite3_mprintf("SELECT * FROM packages WHERE name = '%q' LIMIT 1",
	                        name);
	if (NULL == query) {
		goto end;
	}

	/* unset the package name name */
	info->p_name = NULL;

	/* run the query */
	result = _run_query(database, query, _copy_info, info);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* if the package was not found, report failure */
	if (NULL == info->p_name) {
		result = RESULT_NOT_FOUND;
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}

result_t database_set_installation_data(database_t *database,
                                        const package_info_t *info) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != info);

	/* format the query */
	query = sqlite3_mprintf("INSERT INTO packages VALUES ('%q', '%q', '%q', " \
	                        "'%q', '%q', '%q', '%q', NULL)",
	                        info->p_name,
	                        info->p_version,
	                        info->p_desc,
	                        info->p_file_name,
	                        info->p_arch,
	                        info->p_deps,
	                        info->p_reason);
	if (NULL == query) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}

result_t database_set_metadata(database_t *database,
                               const package_info_t *info) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != info);

	/* format the query */
	query = sqlite3_mprintf("INSERT INTO packages VALUES ('%q', '%q', '%q', " \
	                        "'%q', '%q', '%q', NULL)",
	                        info->p_name,
	                        info->p_version,
	                        info->p_desc,
	                        info->p_file_name,
	                        info->p_arch,
	                        info->p_deps);
	if (NULL == query) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}

result_t database_remove_installation_data(database_t *database,
                                           const char *package) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != package);

	log_write(LOG_INFO, "Unregistering %s\n", package);

	/* format the query */
	query = sqlite3_mprintf("DELETE FROM packages WHERE name = '%q'",
	                        package);
	if (NULL == query) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}

result_t database_register_path(database_t *database,
                                const char *path,
                                const char *package) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != path);
	assert(NULL != package);

	log_write(LOG_DEBUG, "Registering %s (%s)\n", path, package);

	/* format the query */
	query = sqlite3_mprintf("INSERT INTO files VALUES ('%q', '%q', NULL)",
	                        package,
	                        path);
	if (NULL == query) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}

result_t database_unregister_path(database_t *database, const char *path) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != path);

	log_write(LOG_DEBUG, "Unregistering %s\n", path);

	/* format the query */
	query = sqlite3_mprintf("DELETE FROM files WHERE path = '%q'", path);
	if (NULL == query) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}

result_t database_for_each_inst_package(database_t *database,
                                        const query_callback_t callback,
                                        void *arg) {
	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != callback);

	/* run the query */
	return _run_query(database, "SELECT * from packages", callback, arg);
}

result_t database_for_each_file(database_t *database,
                                const char *name,
                                const query_callback_t callback,
                                void *arg) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	/* the executed query */
	char *query = NULL;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != name);
	assert(NULL != callback);

	/* format the query */
	query = sqlite3_mprintf(
	                "SELECT * from files WHERE package = '%q' ORDER BY id DESC",
	                name);
	if (NULL == query) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, query, callback, arg);
	if (SQLITE_OK != result) {
		goto free_query;
	}

	/* report success */
	result = RESULT_OK;

free_query:
	/* free the query */
	sqlite3_free(query);

end:
	return result;
}
