#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#include "database.h"

/* the maximum length of an input line */
#define MAX_LINE_LENGTH (1024)

int main(int argc, char *argv[]) {
	/* a reading buffer */
	char buffer[MAX_LINE_LENGTH] = {'\0'};

	/* package metadata */
	package_info_t info = {0};

	/* the output database */
	database_t output = {0};

	/* the length of an input line */
	size_t length = 0;

	/* a loop index */
	unsigned int i = 0;

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* an input line */
	char *line = NULL;

	/* strtok_r()'s position within the line */
	char *position = NULL;

	/* the input file */
	FILE *input = NULL;

	/* make sure a CSV file and a database were specified */
	if (3 != argc) {
		log_dump("Usage: repodude CSV DEST\n");
		goto end;
	}

	/* open the input file */
	log_write(LOG_DEBUG, "Opening %s\n", argv[1]);
	input = fopen(argv[1], "r");
	if (NULL == input) {
		goto end;
	}

	/* if the output file exists, delete it to ensure the newly created database
	 * is clean from any remains */
	(void) unlink(argv[2]);

	/* open the output file */
	log_write(LOG_INFO, "Initializing %s\n", argv[2]);
	if (RESULT_OK != database_open_write(&output,
	                                     DATABASE_TYPE_METADATA,
	                                     argv[2])) {
		goto close_input;
	}

	do {
		/* read an input line */
		line = fgets((char *) &buffer, sizeof(buffer), input);
		if (NULL == line) {
			break;
		}

		length = strlen(line);

		/* skip empty lines */
		if (0 == length) {
			continue;
		};

		/* strip trailing line breaks */
		--length;
		if ('\n' == line[length]) {
			line[length] = '\0';
		}

		/* convert the line into a metadata structure */
		info._fields[0] = strtok_r(line, ",", &position);
		if (NULL == info._fields[0]) {
			log_write(LOG_ERROR, "The input file is corrupt\n");
			goto close_output;
		}
		for (i = 1; (METADATA_FIELDS_COUNT - 1) > i; ++i) {
			info._fields[i] = strtok_r(NULL, ",", &position);
			if (NULL == info._fields[i]) {
				goto close_output;
			}
		}

		/* add a database row */
		log_write(LOG_INFO, "Adding %s\n", info.p_name);
		if (RESULT_OK != database_set_metadata(&output, &info)) {
			log_write(LOG_ERROR, "Failed to add %s\n", info.p_name);
			goto close_output;
		}
	} while (1);

	/* report success */
	exit_code = EXIT_SUCCESS;

close_output:
	/* close the database */
	(void) database_close(&output);

close_input:
	/* close the input file */
	(void) fclose(input);

end:
	return exit_code;
}