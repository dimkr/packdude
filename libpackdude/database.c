#include "database.h"

bool database_open(database_t *database) {
	/* the return value */
	bool is_success = false;

	/* initialize the database fetcher */
	if (false == fetcher_open(&database->fetcher))
		goto end;

	/* report success */
	is_success = true;

end:
	return is_success;
}

bool database_query_by_name(database_t *database, const char *name, char **raw_metadata, size_t *size) {
	/* the return value */
	bool is_success = false;

	/* the URL */
	char url[MAX_URL_LENGTH];

	/* the fetcher buffer */
	fetcher_buffer_t buffer;

	/* fetch the package metadata */
	(void) snprintf((char *) &url, sizeof(url), "%s/%s", REPOSITORY_ROOT, name);
	if (false == fetcher_fetch_to_memory(&database->fetcher, (const char *) &url, &buffer))
		goto end;
	*raw_metadata = (char *) buffer.buffer;
	*size = buffer.size;

	/* report success */
	is_success = true;

end:
	return is_success;
}

void database_close(database_t *database) {
	/* close the database fetcher */
	fetcher_close(&database->fetcher);
}

bool database_download_by_file_name(database_t *database, const char *file_name, const char *destination) {
	/* the return value */
	bool is_success = false;

	/* the URL */
	char url[MAX_URL_LENGTH];

	/* the output file */
	FILE *output;

	/* open the output file */
	output = fopen(destination, "w");
	if (NULL == output)
		goto end;

	/* fetch the package */
	(void) snprintf((char *) &url, sizeof(url), "%s/%s", REPOSITORY_ROOT, file_name);
	if (false == fetcher_fetch_to_file(&database->fetcher, (const char *) &url, output))
		goto close_file;

	/* report success */
	is_success = true;

close_file:
	/* close the output file */
	(void) fclose(output);

end:
	return is_success;
}
