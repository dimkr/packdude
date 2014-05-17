#ifndef _PACKAGE_OPS_H_INCLUDED
#	define _PACKAGE_OPS_H_INCLUDED

#	include "result.h"
#	include "database.h"
#	include "package.h"

/*!
 * @defgroup package_ops "Package Operations"
 * @brief Low-level package operations
 * @{ */

/*!
 * @struct file_register_params_t
 * @brief The parameters of _register_file() */
typedef struct {
	database_t *database; /*!< The database the file gets added to */
	const char *package; /*!< The package associated with the file */
} file_register_params_t;

/*!
 * @fn result_t package_install(const char *name,
 *                              package_t *package,
 *                              database_t *database);
 * @brief Installs a package
 * @param name The package name
 * @param package The package
 * @param database The database the package gets added to */
result_t package_install(const char *name,
                         package_t *package,
                         database_t *database);

/*!
 * @fn result_t package_remove(const char *name, database_t *database);
 * @brief Removes a package
 * @param name The package name
 * @param databasee The database the package gets removed from */
result_t package_remove(const char *name, database_t *database);

/*!
 * @} */

#endif