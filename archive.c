#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <archive.h>
#include <archive_entry.h>

#include "log.h"
#include "archive.h"

#define EXTRACTION_OPTIONS (ARCHIVE_EXTRACT_OWNER | \
                            ARCHIVE_EXTRACT_PERM | \
                            ARCHIVE_EXTRACT_TIME | \
                            ARCHIVE_EXTRACT_UNLINK | \
                            ARCHIVE_EXTRACT_ACL | \
                            ARCHIVE_EXTRACT_FFLAGS | \
                            ARCHIVE_EXTRACT_XATTR)

static result_t _extract_file(struct archive *input, struct archive *output) {
	/* the data block offset */
	__LA_INT64_T offset = 0LL;

	/* the return value */
	result_t result = RESULT_OK;

	/* the data block size */
	size_t size = 0;

	/* a data block */
	const void *block = NULL;

	assert(NULL != input);
	assert(NULL != output);

	do {
		/* read a data block */
		switch (archive_read_data_block(input, &block, &size, &offset)) {
			case ARCHIVE_OK:
				break;

			case ARCHIVE_EOF:
				goto end;

			default:
				result = RESULT_IO_ERROR;
				goto end;
		}

		/* extract the data block */
		if (ARCHIVE_OK != archive_write_data_block(output,
		                                           block,
		                                           size,
		                                           offset)) {
			goto end;
		}
	} while (1);

end:
	return result;
}

result_t archive_extract(unsigned char *contents,
                         const size_t size,
                         const file_callback_t callback,
                         void *arg) {
	/* the return value */
	result_t result = RESULT_MEM_ERROR;

	/* the archive */
	struct archive *input = NULL;

	/* extraction data */
	struct archive *output = NULL;

	/* a file inside the archive */
	struct archive_entry *entry = NULL;

	/* the file path */
	const char *path = NULL;

	assert(NULL != contents);
	assert(0 < size);
	assert(NULL != callback);

	/* allocate memory for reading the archive */
	input = archive_read_new();
	if (NULL == input) {
		goto end;
	}

	/* allocate memory for extracting the archive */
	output = archive_write_disk_new();
	if (NULL == output) {
		goto close_input;
	}

	/* set the reading options */
	archive_read_support_filter_xz(input);
	archive_read_support_format_tar(input);

	/* set the extraction options */
	archive_write_disk_set_options(output, EXTRACTION_OPTIONS);
	archive_write_disk_set_standard_lookup(output);

	/* open the archive */
	if (0 != archive_read_open_memory(input, contents, size)) {
		log_write(LOG_ERROR, "Failed to read the package\n");
		goto close_output;
	}

	do {
		/* read the name of one file inside the archive */
		switch (archive_read_next_header(input, &entry)) {
			case ARCHIVE_OK:
				break;

			case ARCHIVE_EOF:
				result = RESULT_OK;
				goto close_output;

			default:
				log_write(LOG_ERROR, "Failed to read an archive entry\n");
				goto close_output;
		}

		/* get the file path */
		path = archive_entry_pathname(entry);
		if (NULL == path) {
			result = RESULT_CORRUPT_DATA;
			break;
		}

		/* make sure all paths begin with "./" */
		if (0 != strncmp("./", path, 2)) {
			log_write(LOG_ERROR,
			          "The archive is corrupt; it contains absolute paths\n");
			result = RESULT_CORRUPT_DATA;
			break;
		}

		/* ignore the root directory */
		if (0 == strcmp("./", path)) {
			continue;
		}

		/* call the callback */
		result = callback(path, arg);
		if (RESULT_OK != result) {
			break;
		}

		/* extract the file */
		if (ARCHIVE_OK != archive_write_header(output, entry)) {
			break;
		}
		result = _extract_file(input, output);
		if (RESULT_OK != result) {
			break;
		}
	} while (1);

close_output:
	/* free all memory used for output */
	(void) archive_write_close(output);
	archive_write_free(output);

close_input:
	/* free all memory used for reading the archive */
	(void) archive_read_close(input);
	archive_read_free(input);

end:
	return result;
}
