#ifndef _RESULT_H_INCLUDED
#	define _RESULT_H_INCLUDED

/*!
 * @defgroup result Result
 * @brief Common return values
 * @{ */

/*!
 * @typedef result_t
 * @brief A status code */
typedef unsigned int result_t;

enum results {
	RESULT_OK                = 0,
	RESULT_MEM_ERROR         = 1,
	RESULT_IO_ERROR          = 2,
	RESULT_NETWORK_ERROR     = 3,
	RESULT_CORRUPT_DATA      = 4,
	RESULT_INCOMPATIBLE      = 5,
	RESULT_YES               = 6,
	RESULT_NO                = 7,
	RESULT_ALREADY_INSTALLED = 8,
	RESULT_DATABASE_ERROR    = 9,
	RESULT_ABORTED           = 10,
	RESULT_NOT_FOUND         = 11
};

/*!
 * @} */

#endif