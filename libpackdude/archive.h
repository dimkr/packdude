#ifndef _ARCHIVE_H_INCLUDED
#	define _ARCHIVE_H_INCLUDED

#	include <stdbool.h>
#	include <archive.h>

typedef struct {
        struct archive *archive;
        struct archive *extractor;
} archive_t;

bool archive_open(unsigned char *buffer, const size_t size, archive_t *archive);
void archive_close(archive_t *archive);
bool archive_extract(archive_t *archive, const char *destination);

#endif
