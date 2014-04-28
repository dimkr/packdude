#ifndef _DATABASE_H_INCLUDED
#	define _DATABASE_H_INCLUDED

#	include <stdbool.h>
#	include <stdint.h>
#	include "fetch.h"

#	define REPOSITORY_ROOT "ftp://dimakrasner.com/packdude"

typedef struct {
	fetcher_t fetcher;
} database_t;

bool database_open(database_t *database);
bool database_query_by_name(database_t *database, const char *name, char **raw_metadata, size_t *size);
bool database_download_by_file_name(database_t *database, const char *file_name, const char *destination);
void database_close(database_t *database);

#endif
