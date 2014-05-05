#ifndef _REPO_H_INCLUDED
#	define _REPO_H_INCLUDED

#	include <stdio.h>
#	include "result.h"
#	include "database.h"
#	include "fetch.h"

#	define REPO_DATABASE_FILE_NAME "repo.sqlite3"
#	define METADATA_DATABASE_PATH "."VAR_DIR"/packdude/"REPO_DATABASE_FILE_NAME

typedef struct {
	const char *url;
	fetcher_t fetcher;
} repo_t;

result_t repo_open(repo_t *repo, const char *url);
result_t repo_get_database(repo_t *repo, database_t *database);
result_t repo_get_package(repo_t *repo, package_info_t *info, const char *path);
void repo_close(repo_t *repo);

#endif