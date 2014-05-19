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

result_t _get_file(repo_t *repo, const char *file_name, const char *path) {
	/* the file URL */
	char url[MAX_URL_LENGTH] = {'\0'};

	/* the local file */
	FILE *file = NULL;

	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != repo);
	assert(NULL != file_name);
	assert(NULL != path);

	/* format the file URL */
	if (sizeof(url) <= snprintf((char *) &url,
	                            sizeof(url),
	                            "%s/%s",
	                            repo->url,
	                            file_name)) {
		goto end;
	}

	/* open the file for writing */
	file = fopen(path, "w");
	if (NULL == file) {
		result = RESULT_IO_ERROR;
		goto end;
	}

	/* fetch the file */
	result = fetcher_fetch_to_file(&repo->fetcher, (const char *) &url, file);
	if (RESULT_OK != result) {
		goto close_file;
	}

	/* report success */
	result = RESULT_OK;

close_file:
	/* close the file */
	(void) fclose(file);

end:
	return result;
}

result_t repo_get_database(repo_t *repo, database_t *database) {
	/* the dayabase path */
	char path[PATH_MAX] = {'\0'};

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
		if (MAX_METADATA_CACHE_AGE > (time(NULL) - attributes.st_mtim.tv_sec)) {
			log_write(LOG_DEBUG, "Using the package database cache\n");
			goto open_database;
		}
	}

	/* fetch the database */
	log_write(LOG_INFO, "Fetching the package database from %s\n", repo->url);
	result = _get_file(repo, REPO_DATABASE_FILE_NAME, (const char *) &path);
	if (RESULT_OK != result) {
		goto end;
	}

open_database:
	/* open it */
	result = database_open_read(database, (const char *) &path);
	if (RESULT_OK != result) {
		result = RESULT_DATABASE_ERROR;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t repo_get_package(repo_t *repo,
                          package_info_t *info,
                          const char *path) {
	assert(NULL != repo);
	assert(NULL != info);
	assert(NULL != info->p_file_name);
	assert(NULL != path);

	return _get_file(repo, info->p_file_name, path);
}