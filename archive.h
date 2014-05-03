#ifndef _ARCHIVE_H_INCLUDED
#	define _ARCHIVE_H_INCLUDED

#	include <sys/types.h>
#	include "result.h"

typedef result_t (*file_callback_t)(const char *path, void *arg);

result_t archive_extract(unsigned char *contents,
                         const size_t size,
                         const file_callback_t callback,
                         void *arg);

#endif