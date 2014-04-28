#ifndef _FETCH_H_INCLUDED
#	define _FETCH_H_INCLUDED

#	include <stdbool.h>
#	include <stdio.h>
#	include <curl/curl.h>

#	define MAX_URL_LENGTH (1024)

typedef struct {
	CURL *handle;
} fetcher_t;

typedef struct {
	unsigned char *buffer;
	size_t size;
} fetcher_buffer_t;

bool fetcher_open(fetcher_t *fetcher);
bool fetcher_fetch_to_file(fetcher_t *fetcher, const char *url, FILE *destination);
bool fetcher_fetch_to_memory(fetcher_t *fetcher,
                             const char *url,
                             fetcher_buffer_t *buffer);
void fetcher_close(fetcher_t *fetcher);

#endif
