#include "database.h"

result_t database_open(database_t *database) {
	/* the return value */
	result_t result;

	/* initialize the database fetcher */
	result = fetcher_open(&database->fetcher);
	if (RESULT_OK != result)
		goto end;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t database_query_by_name(database_t *database, const char *name, char **raw_metadata, size_t *size) {
	/* the return value */
	result_t result;

	/* the URL */
	char url[MAX_URL_LENGTH];

	/* the fetcher buffer */
	fetcher_buffer_t buffer;

	/* fetch the package metadata */
	(void) snprintf((char *) &url, sizeof(url), "%s/%s", REPOSITORY_ROOT, name);
	result = fetcher_fetch_to_memory(&database->fetcher, (const char *) &url, &buffer);
	if (RESULT_OK != result)
		goto end;
	*raw_metadata = (char *) buffer.buffer;
	*size = buffer.size;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void database_close(database_t *database) {
	/* close the database fetcher */
	fetcher_close(&database->fetcher);
}

result_t database_download_by_file_name(database_t *database, const char *file_name, const char *destination) {
	/* the return value */
	result_t result = RESULT_FOPEN_FAILED;

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
	result = fetcher_fetch_to_file(&database->fetcher, (const char *) &url, output);
	if (RESULT_OK != result)
		goto close_file;

	/* report success */
	result = RESULT_OK;

close_file:
	/* close the output file */
	(void) fclose(output);

end:
	return result;
}
