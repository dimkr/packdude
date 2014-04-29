#ifndef _ARCHIVE_H_INCLUDED
#	define _ARCHIVE_H_INCLUDED

#	include <archive.h>
#	include "error.h"
#	include "flist.h"

typedef struct {
        struct archive *archive;
        struct archive *extractor;
} archive_t;

result_t archive_open(unsigned char *buffer, const size_t size, archive_t *archive);
void archive_close(archive_t *archive);
result_t archive_extract(archive_t *archive, file_list_t *file_list, const char *destination);

#endif
