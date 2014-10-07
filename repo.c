#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zlib.h>

#include "log.h"
#include "repo.h"

result_t repo_open(repo_t *repo, const char *url) {
	/* the return value */
	result_t result = RESULT_MEM_ERROR;

	assert(NULL != repo);
	assert(NULL != url);

	/* initialize the repository fetcher */
	log_write(LOG_DEBUG, "Connecting to the repository at %s\n", url);
	result = fetcher_new(&repo->fetcher);
	if (RESULT_OK != result) {
		goto end;
	}

	/* save the repository URL */
	repo->url = url;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void repo_close(repo_t *repo) {
	assert(NULL != repo);

	/* free the fetcher */
	log_write(LOG_DEBUG, "Disconnecting from %s\n", repo->url);
	fetcher_free(&repo->fetcher);
}

result_t repo_get_database(repo_t *repo, database_t *database) {
	/* the dayabase path */
	char path[PATH_MAX] = {'\0'};

	/* the database URL */
	char url[MAX_URL_LENGTH] = {'\0'};

	/* the database attributes */
	struct stat attributes = {0};

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != repo);
	assert(NULL != database);

	/* format the database path */
	if (sizeof(path) <= sprintf(path,
	                            METADATA_DATABASE_PATH_FORMAT,
	                            crc32(
	                              crc32(0L, Z_NULL, 0),
	                              (const Bytef *) repo->url,
	                              (uInt) (sizeof(char) * strlen(repo->url))))) {
		goto end;
	}

	/* get the database attributes */
	if (-1 == stat((const char *) &path, &attributes)) {
		if (ENOENT != errno) {
			result = RESULT_IO_ERROR;
			goto end;
		}
	} else {
		/* if the database exists and not too old, do not fetch it again */
		if (MAX_METADATA_CACHE_AGE >
		    (time(NULL) - (time_t) attributes.st_mtime)) {
			log_write(LOG_DEBUG, "Using the package database cache\n");
			goto open_database;
		}
	}

	/* format the database URL */
	if (sizeof(url) <= snprintf((char *) &url,
	                            sizeof(url),
	                            "%s/%s",
	                            repo->url,
	                            REPO_DATABASE_FILE_NAME)) {
		goto end;
	}

	/* fetch the database */
	log_write(LOG_INFO, "Fetching the package database from %s\n", repo->url);
	result = fetcher_fetch_to_file(&repo->fetcher, (const char *) &url, path);
	if (RESULT_OK != result) {
		goto end;
	}

open_database:
	/* open the database */
	result = database_open_read(database, (const char *) &path);
	if (RESULT_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t repo_get_package(repo_t *repo,
                          const package_info_t *info,
                          fetcher_buffer_t *buffer) {
	/* the package URL */
	char url[MAX_URL_LENGTH] = {'\0'};

	assert(NULL != repo);
	assert(NULL != info);
	assert(NULL != info->p_file_name);
	assert(NULL != buffer);

	/* format the package URL */
	if (sizeof(url) <= snprintf((char *) &url,
	                            sizeof(url),
	                            "%s/%s",
	                            repo->url,
	                            info->p_file_name)) {
		return RESULT_CORRUPT_DATA;
	}

	/* fetch the package */
	return fetcher_fetch_to_memory(&repo->fetcher, (const char *) &url, buffer);
}
