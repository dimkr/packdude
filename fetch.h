#ifndef _FETCH_H_INCLUDED
#	define _FETCH_H_INCLUDED

#	include <sys/types.h>
#	include <stdio.h>
#	include <curl/curl.h>
#	include "result.h"

#	define MAX_URL_LENGTH (1024)

#	define __PASTE(x) # x
#	define _PASTE(x) __PASTE(x)
#	define FETCHER_USER_AGENT (PACKAGE"/"_PASTE(VERSION))

typedef struct {
	CURL *handle;
} fetcher_t;

typedef struct {
	unsigned char *buffer;
	size_t size;
} fetcher_buffer_t;

result_t fetcher_new(fetcher_t *fetcher);
result_t fetcher_fetch_to_file(fetcher_t *fetcher,
                               const char *url,
                               FILE *destination);

result_t fetcher_fetch_to_memory(fetcher_t *fetcher,
                                 const char *url,
                                 fetcher_buffer_t *buffer);
result_t fetcher_fetch_compressed_to_memory(fetcher_t *fetcher,
                                            const char *url,
                                            fetcher_buffer_t *buffer);

void fetcher_free(fetcher_t *fetcher);

#endif
