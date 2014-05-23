#ifndef _FETCH_H_INCLUDED
#	define _FETCH_H_INCLUDED

#	include <sys/types.h>

#	include <curl/curl.h>

#	include "result.h"

/*!
 * @defgroup fetch URL Fetching
 * @brief URL-based file download
 * @{ */

/*!
 * @def MAX_URL_LENGTH
 * @brief The maximum length of a URL */
#	define MAX_URL_LENGTH (1024)

#	define __PASTE(x) # x
#	define _PASTE(x) __PASTE(x)

/*!
 * @def FETCHER_USER_AGENT
 * @brief The user agent sent by packdude */
#	define FETCHER_USER_AGENT (PACKAGE"/"_PASTE(VERSION))

/*!
 * @struct fetcher_t
 * @brief Data associated with a URL fetching session */
typedef struct {
	CURL *handle; /*!< The underlying \a cURL handle */
} fetcher_t;

/*!
 * @struct fetcher_buffer_t
 * @brief A dynamically-growing buffer */
typedef struct {
	unsigned char *buffer; /*!< The buffer contents */
	size_t size; /*!< The buffer size */
} fetcher_buffer_t;

/*!
 * @fn result_t fetcher_new(fetcher_t *fetcher)
 * @brief Initializes a URL fetching session
 * @param fetcher The session data
 * @see fetcher_free */
result_t fetcher_new(fetcher_t *fetcher);

/*!
 * @fn void fetcher_free(fetcher_t *fetcher)
 * @brief Ends a URL fetching session
 * @param fetcher The session data
 * @see fetcher_new */
void fetcher_free(fetcher_t *fetcher);

/*!
 * @fn result_t fetcher_fetch_to_file(fetcher_t *fetcher,
 *                                    const char *url,
 *                                    const char *path)
 * @brief Fetches a URL into a file
 * @param fetcher The session data
 * @param url The fetched URL
 * @param path The output file */
result_t fetcher_fetch_to_file(fetcher_t *fetcher,
                               const char *url,
                               const char *path);

/*!
 * @fn result_t fetcher_fetch_to_memory(fetcher_t *fetcher,
 *                                      const char *url,
 *                                      fetcher_buffer_t *buffer)
 * @brief Fetches a URL into dynamically-allocated memory
 * @param fetcher The session data
 * @param url The fetched URL
 * @param buffer The output buffer */
result_t fetcher_fetch_to_memory(fetcher_t *fetcher,
                                 const char *url,
                                 fetcher_buffer_t *buffer);

/*!
 * @} */

#endif
