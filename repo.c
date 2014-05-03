#include <stdlib.h>
#include <assert.h>

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
	log_write(LOG_DEBUG, "Closing %s\n", repo->url);
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
		log_write(LOG_ERROR, "Failed to fetch %s\n", file_name);
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
	/* the return value */
	result_t result = RESULT_DATABASE_ERROR;

	/* fetch the database */
	log_write(LOG_INFO, "Fetching the package database from %s\n", repo->url);
	result = _get_file(repo, REPO_DATABASE_FILE_NAME, METADATA_DATABASE_PATH);
	if (RESULT_OK != result) {
		goto end;
	}

	/* open it */
	result = database_open_read(database, METADATA_DATABASE_PATH);
	if (RESULT_OK != result) {
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
	assert (NULL != info);
	assert (NULL != info->p_file_name);

	return _get_file(repo, info->p_file_name, path);
}