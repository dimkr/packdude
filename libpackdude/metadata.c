#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "metadata.h"

bool metadata_add(metadata_t *metadata, const char *name, const char *value) {
	/* the return value */
	bool is_success = false;

	/* metadata fields */
	metadata_field_t *fields;

	/* enlarge the fields array */
	fields = realloc(metadata->fields, sizeof(metadata_field_t) * (1 + metadata->count));
	if (NULL == fields)
		goto end;
	metadata->fields = fields;

	/* add the field to the array */
	metadata->fields[metadata->count].name = name;
	metadata->fields[metadata->count].value = value;
	++(metadata->count);

	/* report success */
	is_success = true;

end:
	return is_success;
}

bool metadata_parse(const char *raw, const size_t size, metadata_t *metadata) {
	/* the return value */
	bool is_success = false;

	/* the current position within the raw metadata */
	char *position;

	/* a line in the raw metadata */
	char *line;

	/* a metadata field value */
	char *value;

	/* the value length */
	size_t length;

	/* create a terminated, writable copy of the raw metadata */
	metadata->copy = malloc(sizeof(char) * (size_t) (1 + size));
	if (NULL == metadata->copy)
		goto end;
	(void) memcpy(metadata->copy, raw, (size_t) size);
	metadata->copy[size - 1] = '\0';

	line = strtok_r(metadata->copy, "\n", &position);
	if (NULL == line)
		goto free_copy;

	metadata->count = 0;
	metadata->fields = NULL;
	do {
		/* parse the line */
		value = strchr(line, '=');
		if (NULL == value)
			goto free_fields;
		value[0] = '\0';
		++value;
	
		/* calculate the value length */
		length = strlen(value);

		/* trim trailing line breaks */
		if (0 < length) {
			--length;
			if ('\n' == value[length])
				value[length] = '\0';
		}

		/* add it to the array */
		if (false == metadata_add(metadata, line, value))
			goto free_fields;

		/* continue to the next line */
		line = strtok_r(NULL, "\n", &position);
	} while (NULL != line);

	/* report success */
	is_success = true;
	goto end;

free_fields:
	/* free the parsed metadata */
	free(metadata->fields);

free_copy:
	/* free the raw metadata */
	free(metadata->copy);

end:
	return is_success;
}

const char *metadata_get(const metadata_t *metadata, const char *name) {
	/* the return value */
	const char *value = NULL;

	/* a loop index */
	unsigned int i;

	for (i = 0; metadata->count > i; ++i) {
		if (0 == strcmp(metadata->fields[i].name, name)) {
			value = metadata->fields[i].value;
			break;
		}
	}

	return value;
}

void metadata_close(metadata_t *metadata) {
	/* free the raw metadata */
	free(metadata->copy);
}

bool metadata_dump(const metadata_t *metadata, const char *path) {
	/* the return value */
	bool is_success = false;

	/* the file */
	FILE *file;

	/* a loop index */
	unsigned int i;

	/* open the file */
	file = fopen(path, "w");
	if (NULL == file)
		goto end;

	/* write all fields to the file */
	for (i = 0; metadata->count > i; ++i) {
		if (0 > fprintf(file, "%s=%s\n", metadata->fields[i].name, metadata->fields[i].value))
			goto close_file;
	}

	/* report success */
	is_success = true;

close_file:
	/* close the file */
	(void) fclose(file);

end:
	return is_success;
}
