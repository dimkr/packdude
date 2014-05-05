#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

#include "log.h"
#include "fetch.h"

result_t fetcher_new(fetcher_t *fetcher) {
	/* the return value */
	result_t result = RESULT_MEM_ERROR;

	/* initialize an "easy" libcurl session */
	fetcher->handle = curl_easy_init();
	if (NULL == fetcher->handle) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

size_t _append_to_file(char *ptr, size_t size, size_t nmemb, FILE *file) {
	log_write(LOG_DEBUG, "Received a chunk of %u bytes\n", (size * nmemb));
	return fwrite(ptr, size, nmemb, file);
}

result_t fetcher_fetch_to_file(fetcher_t *fetcher,
                               const char *url,
                               FILE *destination) {
	/* the return value */
	result_t result = RESULT_MEM_ERROR;

	log_write(LOG_DEBUG, "Fetching %s\n", url);

	/* set the input URL and the output file */
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEFUNCTION,
	                                 _append_to_file)) {
		goto end;
	}
	if (CURLE_OK != curl_easy_setopt(fetcher->handle, CURLOPT_URL, url)) {
		goto end;
	}
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEDATA,
	                                 destination)) {
		goto end;
	}
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_USERAGENT,
	                                 FETCHER_USER_AGENT)) {
		goto end;
	}

	/* fetch the URL */
	if (CURLE_OK != curl_easy_perform(fetcher->handle)) {
		result = RESULT_NETWORK_ERROR;
		goto end;
	}

	/* flush the output file */
	if (0 != fflush(destination)) {
		result = RESULT_IO_ERROR;
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
	/* the enlarged buffer */
	fetcher_buffer_t new_buffer = {0};

	/* the number of available bytes */
	size_t bytes_available = 0;

	/* the number of bytes handled */
	size_t bytes_handled = 0;

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
	result_t result = RESULT_MEM_ERROR;

	/* set the input URL and the output file */
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEFUNCTION,
	                                 _append_to_buffer)) {
		goto end;
	}
	if (CURLE_OK != curl_easy_setopt(fetcher->handle, CURLOPT_URL, url)) {
		goto end;
	}
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_WRITEDATA,
	                                 buffer)) {
		goto end;
	}
	if (CURLE_OK != curl_easy_setopt(fetcher->handle,
	                                 CURLOPT_USERAGENT,
	                                 FETCHER_USER_AGENT)) {
		goto end;
	}

	/* fetch the URL */
	buffer->buffer = NULL;
	buffer->size = 0;
	if (CURLE_OK != curl_easy_perform(fetcher->handle)) {
		result = RESULT_NETWORK_ERROR;
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}


result_t fetcher_fetch_compressed_to_memory(fetcher_t *fetcher,
                                            const char *url,
                                            fetcher_buffer_t *buffer) {
	/* the return value */
	result_t result = RESULT_MEM_ERROR;

	/* the decompressed data */
	void *decompressed_data = NULL;

	/* the decompressed data size */
	size_t decompressed_size = 0;

	/* fetch the URL */
	result = fetcher_fetch_to_memory(fetcher, url, buffer);
	if (RESULT_OK != result) {
		goto end;
	}

	/* decompress the fetched data */
	decompressed_data = tinfl_decompress_mem_to_heap(buffer->buffer,
	                                                 buffer->size,
	                                                 &decompressed_size,
	                                                 0);

	/* free the compressed data */
	free(buffer->buffer);

	/* upon failure to decompress the data, report failure */
	if (NULL == decompressed_data) {
		goto end;
	}

	/* report success */
	buffer->buffer = decompressed_data;
	buffer->size = decompressed_size;
	result = RESULT_OK;

end:
	return result;
}

void fetcher_free(fetcher_t *fetcher) {
	/* end the libcurl session */
	curl_easy_cleanup(fetcher->handle);
}
