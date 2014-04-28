#ifndef _DATABASE_H_INCLUDED
#	define _DATABASE_H_INCLUDED

#	include <stdint.h>
#	include "fetch.h"
#	include "error.h"

#	define REPOSITORY_ROOT "ftp://dimakrasner.com/packdude"

typedef struct {
	fetcher_t fetcher;
} database_t;

result_t database_open(database_t *database);
result_t database_query_by_name(database_t *database, const char *name, char **raw_metadata, size_t *size);
result_t database_download_by_file_name(database_t *database, const char *file_name, const char *destination);
void database_close(database_t *database);

#endif
