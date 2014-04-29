#ifndef _FETCH_H_INCLUDED
#	define _FETCH_H_INCLUDED

#	include <stdio.h>
#	include <curl/curl.h>
#	include "error.h"

#	define MAX_URL_LENGTH (1024)

typedef struct {
	CURL *handle;
} fetcher_t;

typedef struct {
	unsigned char *buffer;
	size_t size;
} fetcher_buffer_t;

result_t fetcher_open(fetcher_t *fetcher);
result_t fetcher_fetch_to_file(fetcher_t *fetcher, const char *url, FILE *destination);
result_t fetcher_fetch_to_memory(fetcher_t *fetcher,
                             const char *url,
                             fetcher_buffer_t *buffer);
void fetcher_close(fetcher_t *fetcher);

#endif