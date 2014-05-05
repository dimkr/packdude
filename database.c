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

	for ( ; 0 <= i; --i) {
		if (NULL != info->_fields[i]) {
			free(info->_fields[i]);
		}
	}
}

result_t _open_database(database_t *database,
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

	/* save the database path */
	database->path = path;

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

result_t _run_query(database_t *database,
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
			break;

		case SQLITE_ABORT:
			log_write(LOG_DEBUG, "The SQL query has been aborted\n");
			result = RESULT_ABORTED;
			goto end;

		default:
			log_write(LOG_DEBUG, "An SQLite3 error occurred: %s\n", error);
			sqlite3_free(error);
			goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
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

int _copy_info(void *arg, int count, char **values, char **names) {
	/* a loop index */
	int i = 0;

	assert(NULL != arg);
	assert((METADATA_FIELDS_COUNT == count) ||
	       (INSTALLATION_DATA_FIELDS_COUNT == count));
	assert(NULL != values);
	assert(NULL != names);

	for ( ; count > i; ++i) {
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
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != name);
	assert(NULL != info);

	log_write(LOG_DEBUG, "Searching the package database for %s\n", name);

	/* format the query */
	if (sizeof(query) <= snprintf(
		                       (char *) query,
		                       sizeof(query),
	                           "SELECT * FROM packages WHERE name = '%s'",
	                           name)) {
		goto end;
	}

	/* unset the package name name */
	info->p_name = NULL;

	/* run the query */
	result = _run_query(database, (const char *) &query, _copy_info, info);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* if the package was not found, report failure */
	if (NULL == info->p_name) {
		result = RESULT_NOT_FOUND;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_set_installation_data(database_t *database,
                                        const package_info_t *info) {
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != info);

	/* format the query */
	if (sizeof(query) <= snprintf(
		     (char *) query,
	         sizeof(query),
	         "INSERT INTO packages VALUES ('%s', '%s', '%s', '%s', '%s', '%s', NULL)",
	         info->p_name,
	         info->p_version,
	         info->p_file_name,
	         info->p_arch,
	         info->p_deps,
	         info->p_reason)) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, (const char *) &query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_set_metadata(database_t *database, const package_info_t *info) {
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != info);

	/* format the query */
	if (sizeof(query) <= snprintf(
		     (char *) query,
	         sizeof(query),
	         "INSERT INTO packages VALUES ('%s', '%s', '%s', '%s', '%s', NULL)",
	         info->p_name,
	         info->p_version,
	         info->p_file_name,
	         info->p_arch,
	         info->p_deps)) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, (const char *) &query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_remove_installation_data(database_t *database,
                                           const char *package) {
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != package);

	log_write(LOG_INFO, "Unregistering %s\n", package);

	/* format the query */
	if (sizeof(query) <= snprintf((char *) query,
	                              sizeof(query),
	                              "DELETE FROM packages WHERE name = '%s'",
	                              package)) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, (const char *) &query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_register_path(database_t *database,
                                const char *path,
                                const char *package) {
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != path);
	assert(NULL != package);

	log_write(LOG_DEBUG, "Registering %s (%s)\n", path, package);

	/* format the query */
	if (sizeof(query) <= snprintf((char *) query,
	                              sizeof(query),
	                              "INSERT INTO files VALUES ('%s', '%s', NULL)",
	                              package,
	                              path)) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, (const char *) &query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_unregister_path(database_t *database, const char *path) {
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != path);

	log_write(LOG_DEBUG, "Unregistering %s\n", path);

	/* format the query */
	if (sizeof(query) <= snprintf((char *) query,
	                              sizeof(query),
	                              "DELETE FROM files WHERE path = '%s'",
	                              path)) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, (const char *) &query, NULL, NULL);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

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
	/* the executed query */
	char query[MAX_SQL_QUERY_SIZE];

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != database);
	assert(NULL != database->handle);
	assert(NULL != name);
	assert(NULL != callback);

	/* format the query */
	if (sizeof(query) <= snprintf((char *) query,
	                              sizeof(query),
	                              "SELECT * from files WHERE package = '%s' ORDER BY id DESC",
	                              name)) {
		goto end;
	}

	/* run the query */
	result = _run_query(database, (const char *) &query, callback, arg);
	if (SQLITE_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}