#include <unistd.h>
#include <limits.h>
#include <archive.h>
#include <archive_entry.h>
#include "archive.h"

result_t _extract_file(struct archive *input, struct archive *output) {
	/* the return value */
	result_t result = RESULT_READ_BLOCK_FAILED;

	/* a data block */
        const void *block;

	/* the data block size */
        size_t size;

	/* the data block offset */
        off_t offset;

	do {
		/* read a data block */
                switch (archive_read_data_block(input, &block, &size, &offset)) {
                	case ARCHIVE_EOF:
				goto success;

                	case ARCHIVE_OK:
				break;

			default:
				goto end;
		}
	
		/* extract the data block */
                if (ARCHIVE_OK != archive_write_data_block(output, block, size, offset)) {
			result = RESULT_READ_BLOCK_FAILED;
			goto end;
		}
        } while (1);

success:
	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t archive_open(unsigned char *buffer, const size_t size, archive_t *archive) {
	/* the return value */
	result_t result = RESULT_ARCHIVE_READ_NEW_FAILED;

        /* allocate memory for reading the archive */
        archive->archive = archive_read_new();
        if (NULL == archive->archive)
                goto end;

        /* allocate memory for extraction */
        archive->extractor = archive_write_disk_new();
        if (NULL == archive->extractor) {
		result = RESULT_ARCHIVE_WRITE_DISK_NEW_FAILED;
                goto close_archive;
	}

        /* set the extrator options */
        archive_write_disk_set_options(archive->extractor, ARCHIVE_EXTRACT_TIME);
        archive_read_support_filter_xz(archive->archive);
        archive_read_support_format_tar(archive->archive);
        archive_write_disk_set_standard_lookup(archive->extractor);

        /* open the archive */
        if (0 != archive_read_open_memory(archive->archive, buffer, size)) {
		result = RESULT_ARCHIVE_READ_OPEN_MEMORY_FAILED;
                goto close_archive;
	}
	
	/* report success */
	result = RESULT_OK;
	goto end;

close_archive:
	/* close the archive */
	archive_close(archive);

end:
	return result;
}

void archive_close(archive_t *archive) {
	archive_read_close(archive->archive);
        archive_read_free(archive->archive);
}


result_t archive_extract(archive_t *archive, const char *destination) {
	/* the return value */
	result_t result = RESULT_GETCWD_FAILED;

	/* the working directory */
	char working_directory[PATH_MAX];

	/* a file inside the archive */
	struct archive_entry *entry;

	/* get the working directory path */
	if (NULL == getcwd((char *) &working_directory, sizeof(working_directory)))
		goto end;

	/* change the working directory to the extraction destination */
	if (-1 == chdir(destination)) {
		result = RESULT_CHDIR_FAILED;
		goto end;
	}

	do {
		/* read the name of one file inside the archive */
		switch (archive_read_next_header(archive->archive, &entry)) {
			case ARCHIVE_EOF:
				goto success;

			case ARCHIVE_OK:
				break;

			default:
				result = RESULT_READ_NEXT_HEADER_FAILED;
				goto restore_working_directory;
		}
		
		/* extract the file */
		if (ARCHIVE_OK != archive_write_header(archive->extractor, entry)) {
			result = RESULT_WRITE_HEADER_FAILED;
			goto restore_working_directory;
		}
		result = _extract_file(archive->archive, archive->extractor);
		if (RESULT_OK != result)
			goto restore_working_directory;
	} while (1);

success:
	/* report success */
	result = RESULT_OK;

restore_working_directory:
	/* restore the original working directory */
	(void) chdir((const char *) &working_directory);

end:
	return result;
}
