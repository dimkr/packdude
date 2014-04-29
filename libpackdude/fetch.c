#include <string.h>
#include <stdlib.h>
#include "fetch.h"
#include "log.h"

result_t fetcher_open(fetcher_t *fetcher) {
	/* the return value */
	result_t result = RESULT_EASY_INIT_FAILED;

	/* initialize an "easy" libcurl session */
	fetcher->handle = curl_easy_init();
	if (NULL == fetcher->handle)
		goto end;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

size_t _append_to_file(char *ptr,
                        size_t size,
                        size_t nmemb,
                        FILE *file) {
	/* write a progress indicator */
	log_write(".");

	return fwrite(ptr, size, nmemb, file);
}

result_t fetcher_fetch_to_file(fetcher_t *fetcher,
                           const char *url,
                           FILE *destination) {
	/* the return value */
	result_t result = RESULT_EASY_INIT_FAILED;

	/* the return value of curl_easy_perform() */
	CURLcode code;

	/* set the input URL and the output file */
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEFUNCTION,
	                                 _append_to_file))
		goto end;
	if (CURLE_OK != curl_easy_setopt(fetcher->handle, CURLOPT_URL, url))
		goto end;
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEDATA,
	                                 destination))
		goto end;

	/* fetch the URL */
	code = curl_easy_perform(fetcher->handle);
	log_write("\n");
	if (CURLE_OK != code) {
		result = RESULT_EASY_PERFORM_FAILED;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

size_t _append_to_buffer(char *ptr,
                        size_t size,
                        size_t nmemb,
                        fetcher_buffer_t *buffer) {
	/* the number of available bytes */
	size_t bytes_available;

	/* the number of bytes handled */
	size_t bytes_handled = 0;

	/* the enlarged buffer */
	fetcher_buffer_t new_buffer;

	/* calculate the number of available bytes */
	bytes_available = size * nmemb;

	/* enlarge the buffer */
	new_buffer.size = buffer->size + bytes_available;
	new_buffer.buffer = realloc(buffer->buffer, new_buffer.size);
	if (NULL == new_buffer.buffer) {
		free(buffer->buffer);
		goto end;
	}

	/* copy the data to the buffer */
	(void) memcpy(&new_buffer.buffer[buffer->size], ptr, bytes_available);
	(void) memcpy(buffer, &new_buffer, sizeof(fetcher_buffer_t));

	/* report success */
	bytes_handled = bytes_available;

end:
	return bytes_handled;
}

result_t fetcher_fetch_to_memory(fetcher_t *fetcher,
                             const char *url,
                             fetcher_buffer_t *buffer) {
	/* the return value */
	result_t result = RESULT_EASY_SETOPT_FAILED;

	/* set the input URL and the output file */
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEFUNCTION,
	                                 _append_to_buffer))
		goto end;
	if (CURLE_OK != curl_easy_setopt(fetcher->handle, CURLOPT_URL, url))
		goto end;
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEDATA,
	                                 buffer))
		goto end;

	/* fetch the URL */
	buffer->buffer = NULL;
	buffer->size = 0;
	if (CURLE_OK != curl_easy_perform(fetcher->handle)) {
		result = RESULT_EASY_PERFORM_FAILED;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void fetcher_close(fetcher_t *fetcher) {
	/* end the libcurl session */
	curl_easy_cleanup(fetcher->handle);
}
