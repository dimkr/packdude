#ifndef _MANAGER_H_INCLUDED
#	define _MANAGER_H_INCLUDED

#	include "repo.h"
#	include "database.h"
#	include "stack.h"
#	include "result.h"

/*!
 * @defgroup manager Manager
 * @brief High-level package management logic
 * @{ */

/*!
 * @def DEFAULT_PREFIX
 * @brief The default operation prefix */
#	define DEFAULT_PREFIX "/"

/*!
 * @def INSTALLATION_DATA_DATABASE_PATH
 * @brief The installation data database path */
#	define INSTALLATION_DATA_DATABASE_PATH "."VAR_DIR"/packdude/data.sqlite3"

/*!
 * @def LOCK_FILE_PATH
 * @brief The lock file path */
#	define LOCK_FILE_PATH "."VAR_DIR"/packdude/lock"

/*!
 * @def INSTALLATION_REASON_USER
 * @brief The installation reason of packages installed by the user */
#	define INSTALLATION_REASON_USER "user"

/*!
 * @def INSTALLATION_REASON_DEPENDENCY
 * @brief The installation reason of packages installed as dependencies */
#	define INSTALLATION_REASON_DEPENDENCY "dependency"

/*!
 * @def INSTALLATION_REASON_CORE
 * @brief The installation reason of core, non-removable pacakges */
#	define INSTALLATION_REASON_CORE "core"

/*!
 * @def ARCHITECTURE_INDEPENDENT
 * @brief The target architecture of architecture-independent packages */
#	define ARCHITECTURE_INDEPENDENT "all"

/*!
 * @def NO_DEPENDENCIES
 * @brief The contents of an empty dependencies list */
#	define NO_DEPENDENCIES "-"

/*!
 * @struct manager_t
 * @brief A package manager */
typedef struct {
	int lock; /*!< The lock file */
	repo_t repo; /*!< The repository */
	database_t avail_packages; /*!< The package metadata database */
	database_t inst_packages; /*!< The installation data database */
	node_t *inst_stack; /*!< The installation stack */
	const char *prefix; /*!< The package installation prefix */
} manager_t;

/*!
 * @struct manager_cleanup_params_t
 * @brief The parameters of _remove_unneeded() */
typedef struct {
	manager_t *manager; /*!< The package manager */
	unsigned int removed; /*!< The number of removed packages */
} manager_cleanup_params_t;

/*!
 * @typedef dependency_callback_t
 * @brief A callback executed for each dependency */
typedef result_t (*dependency_callback_t)(const char *name, void *arg);

/*!
 * @fn result_t manager_new(manager_t *manager,
 *                          const char *prefix,
 *                          const char *repo)
 * @brief Starts a package manager instance
 * @param manager A package manager
 * @param prefix The package manager operation prefix
 * @param repo The repository URL
 * @see manager_free
 * @see DEFAULT_PREFIX */
result_t manager_new(manager_t *manager, const char *prefix, const char *repo);

/*!
 * @fn void manager_free(manager_t *manager)
 * @brief Shuts down a package manager instance
 * @param manager A package manager
 * @see manager_new */
void manager_free(manager_t *manager);

/*!
 * @fn result_t manager_for_each_dependency(
 *                                         manager_t *manager,
 *                                         const char *name,
 *                                         const dependency_callback_t callback,
 *                                         void *arg)
 * @brief Runs a callback for each dependency of a package
 * @param manager A package manager
 * @param name The package name
 * @param callback The callback to execute
 * @param arg A pointer passed to the callback */
result_t manager_for_each_dependency(manager_t *manager,
                                     const char *name,
                                     const dependency_callback_t callback,
                                     void *arg);

/*!
 * @fn result_t manager_fetch(manager_t *manager,
 *                            const char *name,
 *                            const char *reason)
 * @brief Recursively fetches and installs a package and its dependencies
 * @param manager A package manager
 * @param name The package name
 * @param reason The package installation reason
 * @see INSTALLATION_REASON_USER
 * @see INSTALLATION_REASON_DEPENDENCY */
result_t manager_fetch(manager_t *manager,
                       const char *name,
                       const char *reason);

/*!
 * @fn result_t manager_can_remove(manager_t *manager, const char *name)
 * @brief Determines whether a package can be removed safely
 * @param manager A package manager
 * @param name The package name */
result_t manager_can_remove(manager_t *manager, const char *name);

/*!
 * @fn result_t manager_remove(manager_t *manager, const char *name)
 * @brief Removes a package
 * @param manager A package manager
 * @param name The package name */
result_t manager_remove(manager_t *manager, const char *name);

/*!
 * @fn result_t manager_is_installed(manager_t *manager, const char *name)
 * @brief Determines whether a package is currently installed
 * @param manager A package manager
 * @param name The package name */
result_t manager_is_installed(manager_t *manager, const char *name);

/*!
 * @fn result_t manager_cleanup(manager_t *manager)
 * @brief Removes all unneeded dependencies
 * @param manager A package manager
 * @param name The package name */
result_t manager_cleanup(manager_t *manager);

/*!
 * @fn result_t manager_list_inst(manager_t *manager)
 * @brief Lists installed packages
 * @param manager A package manager */
result_t manager_list_inst(manager_t *manager);

/*!
 * @fn result_t manager_list_avail(manager_t *manager)
 * @brief Lists available packages
 * @param manager A package manager */
result_t manager_list_avail(manager_t *manager);

/*!
 * @fn result_t manager_list_removable(manager_t *manager)
 * @brief Lists packages installed by the user, which can be removed
 * @param manager A package manager */
result_t manager_list_removable(manager_t *manager);

/*!
 * @fn result_t manager_list_files(manager_t *manager, const char *name)
 * @brief Lists all files installed by a package
 * @param manager A package manager
 * @param name The package name */
result_t manager_list_files(manager_t *manager, const char *name);

/*!
 * @} */

#endif
